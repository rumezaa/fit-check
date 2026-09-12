#include <engine/score.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <vector>
#include <cmath>
#include <utility>

namespace
{
    using engine::Aesthetic;
    using engine::Formality;
    using engine::formality_gap;
    using engine::formality_rank;
    using engine::Garment;
    using engine::Lch;
    using engine::Occasion;
    using engine::Length;
    using engine::PaletteTarget;
    using engine::ShapeTarget;
    using engine::Pattern;
    using engine::Silhouette;
    using engine::ScoreRes;

    // ---- tunable constants ----
    constexpr float NEUTRAL_C = 15.0f;     // below this chroma, treat as neutral
    constexpr float MUDDY_L_GAP = 10.0f;   // dL under this = "almost same lightness"
    constexpr float TONAL_HUE_MAX = 15.0f; // hue this close = same family (committed)
    constexpr float MUDDY_HUE_MAX = 45.0f; // hue in (TONAL, this) = adjacent-but-off

    // lightness anchors - the curve holds MONO out to MONO_L_GAP, dips through
    // MID_L_GAP and has climbed back to GREAT by CONTRAST_L_FULL
    constexpr float MONO_L_GAP = 8.0f;       // inside this = essentially monochrome
    constexpr float MID_L_GAP = 18.0f;       // the bottom of the dip
    constexpr float CONTRAST_L_FULL = 34.0f; // by here it reads as crisp contrast

    // hue anchors, used when both pieces carry enough chroma to have a hue
    constexpr float ANALOGOUS_HUE_MAX = 45.0f;  // still one family
    constexpr float CLASH_HUE_CENTER = 90.0f;   // far enough to argue, not to answer
    // complementary sits at 180 apart, and this is the tolerance around it -
    // the same width the analogous band allows at the other end of the circle
    constexpr float COMPLEMENT_HUE_MIN = 135.0f;

    constexpr float GREAT = 40.0f;
    constexpr float OKAY = 10.0f;
    constexpr float MONO = 30.0f;
    constexpr float CLASH = -30.0f;
    constexpr float MUDDY = -20.0f;
    constexpr float TONAL = 25.0f; // two neutrals a few steps of lightness apart

    // recency bands, in days since the piece was last worn
    constexpr int STALE_DAYS = 30; // past this it counts as rested
    constexpr int RECENT_DAYS = 7; // worn this week
    constexpr int JUST_WORN_DAYS = 2;

    constexpr float NEVER_WORN = 20.0f;  // owned and ignored, push it forward
    constexpr float RESTED = 10.0f;      // long enough ago to feel fresh
    constexpr float SETTLING = 0.0f;     // a couple of weeks back, neutral
    constexpr float RECENT = -15.0f;     // worn this week
    constexpr float JUST_WORN = -30.0f;  // worn in the last day or two

    // each dimension the vibe cares about pays out or costs, so matching all three beats matching one
    constexpr float SHAPE_HIT = 15.0f;
    constexpr float SHAPE_MISS = -10.0f;

    // proportion, scored on the pair rather than against the vibe
    constexpr float BALANCED = 15.0f; // volume answered by restraint
    constexpr float STACKED = -15.0f; // volume on volume, no waist left to read

    // what one step outside the occasions formality band costs. graded rather
    // than pass/fail because the filter now hands us pieces that miss the dress
    // code when the closet held nothing that met it - jeans at a date night
    // still have to be ranked against each other
    constexpr float FORMALITY_STRETCH = 30.0f;

    constexpr float LO_SCORE_RANGE = -50.0f;
    constexpr float HI_SCORE_RANGE = 50.0f;

    // low chroma means a neutral color since it sits closer to the grey axis
    bool is_neutral(const Lch &color)
    {
        return color.c < NEUTRAL_C;
    }

    // gives us the hue distance which we use to rate color compatibility together
    float hue_dist(float h1, float h2)
    {
        float d = std::fmod(std::abs(h1 - h2), 360.0f); // wrap it in circular degrees

        return std::min(d, 360.0f - d); // short way around the circle
    }

    float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    // reads a score off a table of (x, score) anchors, straight-lining between
    // them. the anchors are the same numbers the bands used to snap to - we
    // interpolate now instead, so a garment landing either side of what used
    // to be a boundary moves a point or two rather than thirty
    template <std::size_t N>
    float ramp(const std::array<std::pair<float, float>, N> &anchors, float x)
    {
        if (x <= anchors.front().first)
        {
            return anchors.front().second;
        }
        for (std::size_t i = 1; i < N; ++i)
        {
            if (x <= anchors[i].first)
            {
                const float x0 = anchors[i - 1].first, y0 = anchors[i - 1].second;
                const float x1 = anchors[i].first, y1 = anchors[i].second;
                return lerp(y0, y1, (x - x0) / (x1 - x0));
            }
        }
        return anchors.back().second;
    }

    // how much of a say hue gets. zero at a dead grey, one once a piece carries
    // enough chroma to read as a colour - it used to be a hard `chroma < 15`,
    // which is why a navy at 17.5 scored thirty points apart from the same
    // navy at 14
    float chromatic(float chroma)
    {
        return std::clamp(chroma / NEUTRAL_C, 0.0f, 1.0f);
    }

    float map_range(float x)
    {
        if (x < LO_SCORE_RANGE)
        {
            return LO_SCORE_RANGE;
        }
        else if (x > HI_SCORE_RANGE)
        {
            return HI_SCORE_RANGE;
        }
        return x;
    }

    // how rested a piece is - never worn scores best since we own it and its not earning its place
    float rank_recency(const Garment &g, std::chrono::sys_days today)
    {
        if (!g.last_worn.has_value())
        {
            return NEVER_WORN;
        }

        const auto days = (today - *g.last_worn).count();

        if (days <= JUST_WORN_DAYS)
        {
            return JUST_WORN;
        }
        if (days <= RECENT_DAYS)
        {
            return RECENT;
        }
        if (days <= STALE_DAYS)
        {
            return SETTLING;
        }
        return RESTED;
    }

    // averaged like the other axes so one stale piece cant drag up a shirt we wore yesterday
    float score_recency(const Garment &top, const Garment &bottom,
                        std::chrono::sys_days today)
    {
        return (rank_recency(top, today) + rank_recency(bottom, today)) / 2;
    }

    // is it in the vibes list - an empty list means it doesnt care so thats worth nothing, not a miss
    template <typename E>
    float shape_axis(const std::vector<E> &wanted, E actual)
    {
        if (wanted.empty())
        {
            return 0.0f;
        }
        return std::ranges::find(wanted, actual) != wanted.end() ? SHAPE_HIT
                                                                  : SHAPE_MISS;
    }

    // how two silhouettes read together, with no vibe involved. proportion is a
    // relationship, not a property - the same oversized tee reads deliberate
    // over fitted trousers and shapeless over baggy cargos - so this is the one
    // shape term that has to look at both pieces at once
    //
    // its not a volume scale collapsed into a number. flowy on flowy is fine
    // and oversized on oversized isnt, even though both stack volume, so the
    // pairs are spelled out
    //
    // Straight stays neutral in every cell on purpose. its the value a garment
    // gets when nobody has said otherwise, so it must never invent an opinion
    constexpr std::array<std::array<float, 4>, 4> kVolume{{
        //             Fitted     Straight  Flowy      Oversized
        /* Fitted */ {{0.0f, 0.0f, BALANCED, BALANCED}},
        /* Straight */ {{0.0f, 0.0f, 0.0f, 0.0f}},
        /* Flowy */ {{BALANCED, 0.0f, 0.0f, STACKED}},
        /* Oversized */ {{BALANCED, 0.0f, STACKED, STACKED}},
    }};

    float volume_balance(Silhouette top, Silhouette bottom)
    {
        return kVolume[static_cast<std::size_t>(top)][static_cast<std::size_t>(bottom)];
    }

    float rank_shape_fit(const Garment &g, const ShapeTarget &target)
    {
        float score = shape_axis(target.silhouettes, g.silhouette) +
                      shape_axis(target.fabrics, g.fabric);

        // tops have no hemline so we only judge bottoms on length
        if (g.length != Length::NA)
        {
            score += shape_axis(target.lengths, g.length);
        }

        return map_range(score);
    }

    // two questions in one axis: does each piece suit the vibe, and do the two
    // sit right next to each other. conformance is averaged so it stays on the
    // same scale as before, then proportion nudges it - a vibe that asked for a
    // silhouette still wins, but it no longer picks blind between a pair that
    // balances and one that doesnt
    float score_shape(const Garment &top, const Garment &bottom,
                      const ShapeTarget &target)
    {
        const float fit =
            (rank_shape_fit(top, target) + rank_shape_fit(bottom, target)) / 2;

        return map_range(fit + volume_balance(top.silhouette, bottom.silhouette));
    }

    // hue distance -> score. flat and good while two colours share a family,
    // through a trough where they are far enough apart to argue and not far
    // enough to answer each other, back up as they approach opposite
    constexpr std::array<std::pair<float, float>, 5> kHueRamp{{
        {0.0f, GREAT},
        {ANALOGOUS_HUE_MAX, GREAT},
        {CLASH_HUE_CENTER, CLASH},
        {COMPLEMENT_HUE_MIN, GREAT},
        {180.0f, GREAT},
    }};

    // lightness separation -> score, for the part of the judgement hue has no
    // say in. it runs monochrome, through tonal, up to crisp contrast - and
    // never below tonal, because how far apart two lightnesses are says how
    // deliberate a pairing reads, never whether it works
    //
    // there used to be a second ramp here that dipped to MUDDY, for a neutral
    // worn against a colour. that dip is a hue complaint - "close but not the
    // same" - and hue is exactly what the pieces it fired on did not have. a
    // black top on a grey skirt is not an almost-match, it is black on grey
    constexpr std::array<std::pair<float, float>, 5> kLightnessRamp{{
        {0.0f, MONO},
        {MONO_L_GAP, MONO},
        {MID_L_GAP, TONAL},
        {CONTRAST_L_FULL, GREAT},
        {100.0f, GREAT},
    }};

    // using lch scale gives us a more dim look on complementary color using
    // degree dist. in hues and l and c. returns a score in [-50, +50]
    //
    // there are no branches left in here. what used to be three cases behind
    // two hard thresholds - both neutral, one neutral, both chromatic - is now
    // two cross-fades, because every threshold we had was a cliff a garment
    // could land a hair off and lose thirty points to
    float score_color(const Garment &top, const Garment &bottom)
    {
        const float dl = std::abs(top.color.l - bottom.color.l);
        const float d = hue_dist(top.color.h, bottom.color.h); // circular, 0..180

        // the duller piece decides whether hue means anything at all - one
        // dead grey and there is nothing for the other hue to sit against
        const float hue_say = chromatic(std::min(top.color.c, bottom.color.c));

        const float lightness = ramp(kLightnessRamp, dl);

        // the "almost matches" complaint is about hue, so it fades out with
        // hue's say the same way the rest of the hue term does
        const bool muddy =
            dl < MUDDY_L_GAP && d > TONAL_HUE_MAX && d < MUDDY_HUE_MAX;
        const float penalty = muddy ? MUDDY * hue_say : 0.0f;

        return map_range(lerp(lightness, ramp(kHueRamp, d), hue_say) + penalty);
    }

    float score_formality(const Garment &top, const Garment &bottom, const Occasion &occ)
    {
        // find if pieces are consistent with each other - in general a a plain cotton t-shirt w/ silk skirt === clash
        float consistency;

        int spread = std::abs(formality_rank(top.formality) - formality_rank(bottom.formality));

        if (spread == 0)
        {
            consistency = GREAT;
        }
        else if (spread == 1)
        {
            consistency = OKAY;
        }
        else
        {
            consistency = CLASH;
        }

        // find how far they sit from the occasions range. a pair inside it
        // still scores GREAT and one step out still scores OKAY, the same as
        // when this was a yes/no - the steps past that are new, so a top thats
        // one rung under the dress code beats one thats two
        const int gap = formality_gap(top.formality, occ) +
                        formality_gap(bottom.formality, occ);
        const float fit = std::max(CLASH, GREAT - FORMALITY_STRETCH * gap);

        return (consistency + fit) / 2;
    }

    // two pieces in the same pattern read as a set when they look like they
    // came off the same bolt: close in tone, and close in hue if they have any
    // hue to speak of
    //
    // deliberately not score_color. that answers "do these go together", which
    // pays out for contrast as readily as for likeness - orange dots against
    // teal dots score full marks there, and they are many things but they are
    // not a set
    bool reads_as_a_set(const Garment &top, const Garment &bottom)
    {
        if (std::abs(top.color.l - bottom.color.l) >= CONTRAST_L_FULL)
        {
            return false; // too far apart in tone to read as one cloth
        }

        // a hue neither piece really has cannot pull them apart - two offwhites
        // sit 26 degrees away from each other on paper and identical in the eye
        const float hue_say = chromatic(std::min(top.color.c, bottom.color.c));
        return hue_say * hue_dist(top.color.h, bottom.color.h) <= ANALOGOUS_HUE_MAX;
    }

    float score_pattern(const Garment &top, const Garment &bottom)
    {
        Pattern t = top.pattern;
        Pattern b = bottom.pattern;

        if (t == Pattern::Solid && b == Pattern::Solid)
        {
            return OKAY;
        }

        if (t == Pattern::Solid || b == Pattern::Solid)
        {
            return GREAT;
        }

        // both in the same pattern reads one of two ways, and colour is what
        // decides which. two polkadots in the same offwhite is a set, chosen
        // on purpose; the same two polkadots fighting on hue is just busy
        //
        if (t == b)
        {
            return reads_as_a_set(top, bottom) ? GREAT : OKAY;
        }

        return CLASH;
    }

    float rank_aesthetic_fit(const Garment &g, const PaletteTarget &target)
    {
        float score = 0;

        // chroma in wanted band ?
        if (target.chroma_min <= g.color.c && g.color.c <= target.chroma_max)
        {
            score += 20;
        }
        else
        {
            score -= 20;
        }

        // lightness in wanted band for aesthetic ?
        if (target.l_min <= g.color.l && g.color.l <= target.l_max)
        {
            score += 15;
        }
        else
        {
            score -= 15;
        }

        // is hue in wanted arc - if the color is muted we should skip it ?
        if (!is_neutral(g.color))
        {

            if (hue_dist(g.color.h, target.hue_center) <= target.hue_spread)
            {

                score += 15;
            }
            else
            {
                score -= 15;
            }
        }

        return map_range(score);
    }

    float score_aesthetic(const Garment &top, const Garment &bottom, const PaletteTarget &target)
    {
        return (rank_aesthetic_fit(top, target) + rank_aesthetic_fit(bottom, target)) / 2;
    }

} // namespace

namespace engine
{

    float ScoreRes::total() const
    {
        return color * weights.color + formality * weights.formality +
               pattern * weights.pattern + palette * weights.palette +
               recency * weights.recency + shape * weights.shape;
    }

    ScoreRes score_pair(const Garment &top, const Garment &bottom,
                        const Occasion &occ, const Aesthetic &vibe,
                        std::chrono::sys_days today)
    {
        ScoreRes res{};
        res.color = score_color(top, bottom);
        res.formality = score_formality(top, bottom, occ);
        res.pattern = score_pattern(top, bottom);
        res.palette = score_aesthetic(top, bottom, vibe.palette);
        res.recency = score_recency(top, bottom, today);
        res.shape = score_shape(top, bottom, vibe.shape);
        res.weights = vibe.weights;
        return res;
    }

}

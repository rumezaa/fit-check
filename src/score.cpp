#include <engine/score.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
    using engine::Aesthetic;
    using engine::Formality;
    using engine::Garment;
    using engine::Lch;
    using engine::Occasion;
    using engine::PaletteTarget;
    using engine::Pattern;
    using engine::ScoreRes;

    // ---- tunable constants ----
    constexpr float NEUTRAL_C = 15.0f;     // below this chroma, treat as neutral
    constexpr float MUDDY_L_GAP = 10.0f;   // dL under this = "almost same lightness"
    constexpr float TONAL_HUE_MAX = 15.0f; // hue this close = same family (committed)
    constexpr float MUDDY_HUE_MAX = 45.0f; // hue in (TONAL, this) = adjacent-but-off

    // lightness bands used when one piece is neutral
    constexpr float MONO_L_GAP = 8.0f;      // under this = essentially monochrome
    constexpr float CONTRAST_L_GAP = 20.0f; // over this = crisp contrast

    // hue bands used when both pieces are chromatic
    constexpr float ANALOGOUS_HUE_MAX = 45.0f;
    constexpr float CLASH_HUE_MAX = 100.0f;
    constexpr float TRIADIC_HUE_MAX = 150.0f;

    constexpr float GREAT = 40.0f;
    constexpr float OKAY = 10.0f;
    constexpr float MONO = 30.0f;
    constexpr float CLASH = -30.0f;
    constexpr float MUDDY = -20.0f;

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

    int rank_formality(Formality f)
    {
        switch (f)
        {
        case Formality::Casual:
            return 0;
        case Formality::Business:
            return 1;
        case Formality::Elegant:
            return 2;
        }
        return 0;
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

    // using lch scale gives us a more dim look on complementary color using degree dist. in hues and l and c
    // returns a score in [-50, +50]
    float score_color(const Garment &top, const Garment &bottom)
    {
        // ---------- BRANCH 1: at least one neutral ----------
        // hue is meaningless here, so judge on lightness separation only.
        // U-shape: good low (mono), bad middle, good high (contrast)
        if (is_neutral(top.color) || is_neutral(bottom.color))
        {
            float dl = std::abs(top.color.l - bottom.color.l);

            if (dl < MONO_L_GAP)
            {
                return MONO; // essentially monochrome, reads as intentional
            }
            if (dl < CONTRAST_L_GAP)
            {
                return MUDDY; // close-but-different, like navy and purple idk
            }
            return GREAT; // clear contrast, crisp
        }

        // ---------- BRANCH 2: both chromatic, essentially not mutes ----------
        float d = hue_dist(top.color.h, bottom.color.h); // circular, 0..180
        float dl = std::abs(top.color.l - bottom.color.l);

        float hue_score = 0.0f;
        if (d <= ANALOGOUS_HUE_MAX)
        {
            hue_score = GREAT; // mono / analogous
        }
        else if (d <= CLASH_HUE_MAX)
        {
            hue_score = CLASH; // muddy clash zone
        }
        else if (d <= TRIADIC_HUE_MAX)
        {
            hue_score = OKAY; // triadic-ish
        }
        else
        {
            hue_score = GREAT; // complementary
        }

        // lightness only punishes the "almost matches" case: close in L *and*
        // adjacent-but-not-same in hue.
        bool muddy = dl < MUDDY_L_GAP && d > TONAL_HUE_MAX && d < MUDDY_HUE_MAX;
        float l_penalty = muddy ? MUDDY : 0.0f;

        return hue_score + l_penalty;
    }

    float score_formality(const Garment &top, const Garment &bottom, const Occasion &occ)
    {
        // find if pieces are consistent with each other - in general a a plain cotton t-shirt w/ silk skirt === clash
        float consistency;

        int spread = std::abs(rank_formality(top.formality) - rank_formality(bottom.formality));

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

        // find if they fit the occasions range
        float fit;
        int top_rank = rank_formality(top.formality);
        int bott_rank = rank_formality(bottom.formality);
        int occ_min = rank_formality(occ.min_formality);
        int occ_max = rank_formality(occ.max_formality);

        bool top_fit = (occ_min <= top_rank) && (top_rank <= occ_max);
        bool bott_fit = (occ_min <= bott_rank) && (bott_rank <= occ_max);

        if (top_fit && bott_fit)
        {
            fit = GREAT;
        }
        else if (top_fit || bott_fit)
        {
            fit = OKAY;
        }
        else
        {
            fit = CLASH;
        }

        return (consistency + fit) / 2;
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

        // case for the same pattern
        if (t == b)
        {
            return OKAY;
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
               pattern * weights.pattern + palette * weights.palette;
    }

    ScoreRes score_pair(const Garment &top, const Garment &bottom,
                        const Occasion &occ, const Aesthetic &vibe)
    {
        ScoreRes res{};
        res.color = score_color(top, bottom);
        res.formality = score_formality(top, bottom, occ);
        res.pattern = score_pattern(top, bottom);
        res.palette = score_aesthetic(top, bottom, vibe.palette);
        res.weights = vibe.weights;
        return res;
    }

}

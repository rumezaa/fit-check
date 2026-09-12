#include <engine/filter.hpp>
#include <engine/garment.hpp>
#include <engine/request.hpp>
#include <engine/score.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <exception>
#include <format>
#include <iostream>
#include <random>
#include <span>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace
{

    // pool we pick from - small enough a bad outfit never shows up, big enough
    // that we dont get the same answer every run
    //
    // whoever is driving us should ask for this many, so the set we sample the
    // opener out of is the same set they can page through. a pool wider than
    // the shortlist just means sampling something the user cant get back to
    constexpr std::size_t kPoolSize = 5;

    // the colour a pair has to beat to be worth showing at all. OKAY is what
    // score_color returns when two colours are triadic-ish - far enough apart
    // not to clash, not far enough to read as deliberate - so it means "no
    // opinion", and no opinion is not a recommendation
    //
    // this is the one bar that means the same thing in every run. colour is
    // scored on the pair alone, so unlike the total, no vibe can weight its
    // way past it - a pair that misses here misses under every vibe
    constexpr float kColorFloor = 10.0f;

    // a scored combo plus the pieces that made it so we can name them when
    // printing - the pointers borrow from Candidates which outlives them
    struct RankedPair
    {
        engine::ScoreRes score;
        float total = 0.0f;
        const engine::Garment *top = nullptr;
        const engine::Garment *bottom = nullptr;
    };

    // spans not Candidates so styling around one piece can pass that piece as a
    // rail of one and reuse the same loop
    std::vector<RankedPair> rank_pairs(std::span<const engine::Garment> tops,
                                       std::span<const engine::Garment> bottoms,
                                       const engine::Occasion &occasion,
                                       const engine::Aesthetic &vibe,
                                       std::chrono::sys_days today)
    {
        std::vector<RankedPair> sorted_ranked_pairs;
        sorted_ranked_pairs.reserve(tops.size() * bottoms.size());

        // we take the cartesian products and get the score
        for (const engine::Garment &top : tops)
        {
            for (const engine::Garment &bottom : bottoms)
            {
                RankedPair pair;
                pair.score = engine::score_pair(top, bottom, occasion, vibe, today);
                pair.total = pair.score.total();
                pair.top = &top;
                pair.bottom = &bottom;
                sorted_ranked_pairs.push_back(pair);
            }
        }

        std::ranges::sort(sorted_ranked_pairs, std::ranges::greater{}, &RankedPair::total);
        return sorted_ranked_pairs;
    }

    // the pairs worth putting in front of someone. we rank first and cut
    // after, so pairs_scored still reports everything we actually looked at
    std::vector<RankedPair> above_color_floor(std::vector<RankedPair> ranked)
    {
        std::erase_if(ranked, [](const RankedPair &pair)
                      { return pair.score.color <= kColorFloor; });
        return ranked;
    }

    // picks one outfit from the best few, leaning towards the better ones but
    // never stuck on the top score so we get a different answer each run
    //
    // we weight by rank not by total - a total based weight makes the bias
    // depend on how spread out the pool is, ranks give us the same odds either
    // way and the best pair stays kPoolSize times likelier than the last
    std::size_t select(const std::vector<RankedPair> &sorted_ranked_pairs,
                       std::mt19937 &rng)
    {
        const std::size_t pool = std::min(kPoolSize, sorted_ranked_pairs.size());

        std::vector<double> odds(pool);
        for (std::size_t i = 0; i < pool; ++i)
        {
            odds[i] = static_cast<double>(pool - i);
        }

        std::discrete_distribution<std::size_t> pick(odds.begin(), odds.end());
        return pick(rng);
    }

    const char *formality_name(engine::Formality f)
    {
        switch (f)
        {
        case engine::Formality::Casual:
            return "Casual";
        case engine::Formality::Business:
            return "Business";
        case engine::Formality::Elegant:
            return "Elegant";
        }
        return "Casual";
    }

    // formality rides along with every piece we name. a stretched outfit is
    // only explainable if whoever renders it can see what the piece actually
    // is next to what the occasion asked for
    json garment_json(const engine::Garment &g)
    {
        return json{{"id", g.id},
                    {"name", g.name},
                    {"hex", g.hex},
                    {"formality", formality_name(g.formality)}};
    }

    const char *category_name(engine::Category c)
    {
        switch (c)
        {
        case engine::Category::Top:
            return "Top";
        case engine::Category::Bottom:
            return "Bottom";
        case engine::Category::Shoes:
            return "Shoes";
        }
        return "Top";
    }

    // we keep the per axis breakdown in the payload on purpose - the total
    // alone doesnt tell us which constant to fix when a pairing looks wrong
    json pair_json(const RankedPair &pair, std::size_t rank)
    {
        return json{
            {"rank", rank},
            {"total", pair.total},
            {"top", garment_json(*pair.top)},
            {"bottom", garment_json(*pair.bottom)},
            {"scores", {{"color", pair.score.color}, {"formality", pair.score.formality}, {"pattern", pair.score.pattern}, {"palette", pair.score.palette}, {"recency", pair.score.recency}, {"shape", pair.score.shape}}}};
    }

    // every exit goes through here so whoever is reading our stdout always
    // gets json back, never a bare message
    int fail(const std::string &message)
    {
        std::cout << json{{"ok", false}, {"error", message}}.dump() << "\n";
        return 1;
    }

} // namespace

int main()
{
    engine::Request req;
    try
    {
        req = engine::read_request(std::cin);
    }
    catch (const std::exception &e)
    {
        return fail(std::string{"bad request: "} + e.what());
    }

    try
    {
        const engine::Occasion &occasion = req.occasion;
        const engine::Aesthetic &vibe = req.vibe;

        // one clock read for the whole run so everything is measured against the same day
        const std::chrono::sys_days today =
            std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());

        // the anchor is the piece the user asked us to style around. we resolve
        // it before filtering because it is exempt from the filter - they can
        // see it in their closet and they picked it, so telling them its too
        // warm for today or needs a wash would just be refusing the question
        const engine::Garment *anchor = nullptr;
        if (req.anchor_id)
        {
            anchor = engine::find_garment(req.closet, *req.anchor_id);
            if (anchor == nullptr)
            {
                return fail(std::format("no garment with id {} in the closet",
                                        *req.anchor_id));
            }
            if (anchor->category == engine::Category::Shoes)
            {
                return fail("can only style around a top or a bottom");
            }
        }

        const engine::Candidates candidates =
            engine::filter_closet(req.closet, occasion, req.weather);
        const engine::WarmthRange warmth = engine::warmth_for(req.weather);

        // anchored, one side of every pair is fixed, so we hand that side in as
        // a rail of one and let the other side come off the filtered closet
        const std::span<const engine::Garment> anchor_rail{anchor, anchor ? 1u : 0u};
        const bool anchored_top = anchor && anchor->category == engine::Category::Top;

        const std::vector<RankedPair> scored = rank_pairs(
            anchored_top ? anchor_rail : std::span<const engine::Garment>{candidates.tops},
            anchor && !anchored_top ? anchor_rail : std::span<const engine::Garment>{candidates.bottoms},
            occasion, vibe, today);

        // everything downstream works off the survivors - we would rather hand
        // back fewer looks, or none, than dress someone in a pair whose
        // colours we have nothing good to say about
        const std::vector<RankedPair> sorted_ranked_pairs = above_color_floor(scored);

        // the relaxed report has to describe the outfit we actually built, not
        // the shape of the closet. the rail the anchor fixes never gets walked,
        // so whatever filter_closet did to refill it is not part of this answer
        // - and the anchor came in exempt from the dress code, so if it misses
        // it, that is the stretch we owe the user instead
        engine::Relaxed relaxed = candidates.relaxed;
        if (anchor != nullptr)
        {
            (anchored_top ? relaxed.tops : relaxed.bottoms) = false;
            relaxed.anchor =
                engine::formality_gap(anchor->formality, occasion) > 0;
        }

        json out{
            {"ok", true},
            {"mode", anchor ? "anchored" : "outfit"},
            {"anchor", anchor ? json{{"id", anchor->id}, {"name", anchor->name}, {"hex", anchor->hex}, {"category", category_name(anchor->category)}, {"formality", formality_name(anchor->formality)}} : json(nullptr)},
            {"occasion", occasion.name},
            // the band itself, not just its name - the relaxed flags below say
            // that we bent it, and this is what says what we bent it to
            {"dress_code", {{"min", formality_name(occasion.min_formality)}, {"max", formality_name(occasion.max_formality)}}},
            {"vibe", vibe.name},
            {"weather", {{"temp_c", req.weather.temp_c}, {"warmth_min", warmth.min}, {"warmth_max", warmth.max}}},
            {"candidates", {{"tops", candidates.tops.size()}, {"bottoms", candidates.bottoms.size()}, {"shoes", candidates.shoes.size()}, {"complete", candidates.complete()}, {"relaxed_formality", {{"tops", relaxed.tops}, {"bottoms", relaxed.bottoms}, {"shoes", relaxed.shoes}, {"anchor", relaxed.anchor}, {"any", relaxed.any()}}}}},
            {"pairs_scored", scored.size()},
            // how many of those the colour bar took. the difference between
            // "your closet had nothing to pair" and "none of it went together"
            // is the difference between two very different things to tell you
            {"dropped_on_color", scored.size() - sorted_ranked_pairs.size()},
        };

        if (sorted_ranked_pairs.empty())
        {
            // nothing to wear isnt an error, its an answer
            out["pick"] = nullptr;
            out["ranked"] = json::array();
            std::cout << out.dump() << "\n";
            return 0;
        }

        std::mt19937 rng{req.seed ? *req.seed : std::random_device{}()};
        const std::size_t picked = select(sorted_ranked_pairs, rng);
        out["pick"] = pair_json(sorted_ranked_pairs[picked], picked + 1);

        // the pool we sample from runs deeper than limit, so a shortlist cut
        // at limit would leave whoever reads pick holding a pair that isnt in
        // the list we handed them. we extend far enough to reach it instead of
        // reordering, so ranked stays in rank order either way
        const std::size_t shortlist = std::max(req.limit, picked + 1);

        json ranked = json::array();
        for (std::size_t i = 0; i < sorted_ranked_pairs.size() && i < shortlist; ++i)
        {
            ranked.push_back(pair_json(sorted_ranked_pairs[i], i + 1));
        }
        out["ranked"] = ranked;

        std::cout << out.dump() << "\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        return fail(e.what());
    }
}

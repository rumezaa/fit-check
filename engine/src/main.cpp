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
    constexpr std::size_t kPoolSize = 7;

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

    json garment_json(const engine::Garment &g)
    {
        return json{{"id", g.id}, {"name", g.name}, {"hex", g.hex}};
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

        const std::vector<RankedPair> sorted_ranked_pairs = rank_pairs(
            anchored_top ? anchor_rail : std::span<const engine::Garment>{candidates.tops},
            anchor && !anchored_top ? anchor_rail : std::span<const engine::Garment>{candidates.bottoms},
            occasion, vibe, today);

        json out{
            {"ok", true},
            {"mode", anchor ? "anchored" : "outfit"},
            {"anchor", anchor ? json{{"id", anchor->id}, {"name", anchor->name}, {"hex", anchor->hex}, {"category", category_name(anchor->category)}} : json(nullptr)},
            {"occasion", occasion.name},
            {"vibe", vibe.name},
            {"weather", {{"temp_c", req.weather.temp_c}, {"warmth_min", warmth.min}, {"warmth_max", warmth.max}}},
            {"candidates", {{"tops", candidates.tops.size()}, {"bottoms", candidates.bottoms.size()}, {"shoes", candidates.shoes.size()}, {"complete", candidates.complete()}, {"relaxed_formality", {{"tops", candidates.relaxed.tops}, {"bottoms", candidates.relaxed.bottoms}, {"shoes", candidates.relaxed.shoes}, {"any", candidates.relaxed.any()}}}}},
            {"pairs_scored", sorted_ranked_pairs.size()},
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

        json ranked = json::array();
        for (std::size_t i = 0; i < sorted_ranked_pairs.size() && i < req.limit; ++i)
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

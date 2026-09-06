#include <engine/filter.hpp>
#include <engine/garment.hpp>
#include <engine/request.hpp>
#include <engine/score.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <exception>
#include <iostream>
#include <random>
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

    std::vector<RankedPair> rank_pairs(const engine::Candidates &candidates,
                                       const engine::Occasion &occasion,
                                       const engine::Aesthetic &vibe,
                                       std::chrono::sys_days today)
    {
        std::vector<RankedPair> sorted_ranked_pairs;
        sorted_ranked_pairs.reserve(candidates.tops.size() * candidates.bottoms.size());

        // we take the cartesian products and get the score
        for (const engine::Garment &top : candidates.tops)
        {
            for (const engine::Garment &bottom : candidates.bottoms)
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

    // we keep the per axis breakdown in the payload on purpose - the total
    // alone doesnt tell us which constant to fix when a pairing looks wrong
    json pair_json(const RankedPair &pair, std::size_t rank)
    {
        return json{
            {"rank", rank},
            {"total", pair.total},
            {"top", garment_json(*pair.top)},
            {"bottom", garment_json(*pair.bottom)},
            {"scores", {{"color", pair.score.color},
                        {"formality", pair.score.formality},
                        {"pattern", pair.score.pattern},
                        {"palette", pair.score.palette},
                        {"recency", pair.score.recency},
                        {"shape", pair.score.shape}}}};
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

        const engine::Candidates candidates =
            engine::filter_closet(req.closet, occasion, req.weather);
        const engine::WarmthRange warmth = engine::warmth_for(req.weather);

        const std::vector<RankedPair> sorted_ranked_pairs =
            rank_pairs(candidates, occasion, vibe, today);

        json out{
            {"ok", true},
            {"occasion", occasion.name},
            {"vibe", vibe.name},
            {"weather", {{"temp_c", req.weather.temp_c},
                         {"warmth_min", warmth.min},
                         {"warmth_max", warmth.max}}},
            {"candidates", {{"tops", candidates.tops.size()},
                            {"bottoms", candidates.bottoms.size()},
                            {"shoes", candidates.shoes.size()},
                            {"complete", candidates.complete()}}},
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

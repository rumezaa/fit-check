#include <engine/filter.hpp>
#include <engine/garment.hpp>
#include <engine/loader.hpp>
#include <engine/score.hpp>

#include <algorithm>
#include <format>
#include <iostream>
#include <vector>

namespace
{

    // how many of the best pairs to print per occasion/aesthetic
    constexpr std::size_t kShowBest = 3;

    void show(const char *label, const std::vector<engine::Garment> &group)
    {
        std::cout << "  " << label << " (" << group.size() << "): ";
        if (group.empty())
        {
            std::cout << "-- none --";
        }
        for (std::size_t i = 0; i < group.size(); ++i)
        {
            std::cout << (i > 0 ? ", " : "") << group[i].name;
        }
        std::cout << "\n";
    }

    // A scored combination, kept alongside the garments it came from so the
    // pieces can be named when printing. The pointers borrow from the
    // Candidates, which outlives every RankedPair built from it.
    struct RankedPair
    {
        engine::ScoreRes score;
        float total = 0.0f;
        const engine::Garment *top = nullptr;
        const engine::Garment *bottom = nullptr;
    };

    // Scores every top x bottom combination, best first. The closet is small,
    // so the full cross product is cheaper than being clever about it.
    std::vector<RankedPair> rank_pairs(const engine::Candidates &candidates,
                                       const engine::Occasion &occasion,
                                       const engine::Aesthetic &vibe)
    {
        std::vector<RankedPair> ranked;
        ranked.reserve(candidates.tops.size() * candidates.bottoms.size());

        // we take the cartesian products and get the score
        for (const engine::Garment &top : candidates.tops)
        {
            for (const engine::Garment &bottom : candidates.bottoms)
            {
                RankedPair pair;
                pair.score = engine::score_pair(top, bottom, occasion, vibe);
                pair.total = pair.score.total();
                pair.top = &top;
                pair.bottom = &bottom;
                ranked.push_back(pair);
            }
        }

        std::ranges::sort(ranked, std::ranges::greater{}, &RankedPair::total);
        return ranked;
    }

    // The per-axis breakdown is here on purpose: the weighted total alone
    // tells you nothing about which constant to reach for when a pairing
    // scores badly.
    void print_pair(const RankedPair &pair)
    {
        std::cout << std::format(
            "    {:>7.1f}  {:<20} + {:<20} [col {:>6.1f}  frm {:>6.1f}  "
            "pat {:>6.1f}  pal {:>6.1f}]\n",
            pair.total, pair.top->name, pair.bottom->name, pair.score.color,
            pair.score.formality, pair.score.pattern, pair.score.palette);
    }

} // namespace

int main(int argc, char **argv)
{
    const std::string closetPath = (argc > 1) ? argv[1] : "fixtures/closet.json";
    const std::string occasionPath = (argc > 2) ? argv[2] : "fixtures/occasions.json";
    const std::string aestheticPath = (argc > 3) ? argv[3] : "fixtures/aesthetics.json";

    const std::vector<engine::Garment> closet = engine::load_closet(closetPath);
    if (closet.empty())
    {
        std::cerr << "no garments loaded\n";
        return 1;
    }

    const std::vector<engine::Occasion> occasions = engine::load_occasions(occasionPath);
    if (occasions.empty())
    {
        std::cerr << "no occasions loaded\n";
        return 1;
    }

    const std::vector<engine::Aesthetic> vibes = engine::load_aesthetics(aestheticPath);
    if (vibes.empty())
    {
        std::cerr << "no vibes loaded\n";
        return 1;
    }

    std::cout << "closet: " << closet.size() << " garments, "
              << occasions.size() << " occasions, "
              << vibes.size() << " vibes\n\n";

    for (const engine::Occasion &occasion : occasions)
    {
        const engine::Candidates c = engine::filter_closet(closet, occasion);
        std::cout << occasion.name << " -- " << c.total() << " of "
                  << closet.size() << " garments pass"
                  << (c.complete() ? "" : "   [INCOMPLETE: no outfit possible]")
                  << "\n";
        show("tops   ", c.tops);
        show("bottoms", c.bottoms);
        show("shoes  ", c.shoes);

        // Scoring is top/bottom only for now, so an empty shoe rack still
        // leaves pairs worth ranking even though it blocks a real outfit.
        if (c.tops.empty() || c.bottoms.empty())
        {
            std::cout << "  no pairs to score\n\n";
            continue;
        }

        for (const engine::Aesthetic &vibe : vibes)
        {
            const std::vector<RankedPair> ranked = rank_pairs(c, occasion, vibe);
            std::cout << std::format("\n  {} -- best {} of {} pairs\n", vibe.name,
                                     std::min(kShowBest, ranked.size()),
                                     ranked.size());

            for (std::size_t i = 0; i < ranked.size() && i < kShowBest; ++i)
            {
                print_pair(ranked[i]);
            }
        }
        std::cout << "\n";
    }
    return 0;
}

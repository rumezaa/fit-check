#include <engine/filter.hpp>
#include <engine/garment.hpp>
#include <engine/loader.hpp>

#include <iostream>

namespace
{

    void show(const engine::Candidates &c, const char *label,
              const std::vector<engine::Garment> &group)
    {
        std::cout << "  " << label << " (" << group.size() << "): ";
        if (group.empty())
        {
            std::cout << "-- none --";
        }
        for (const engine::Garment &g : group)
        {
            std::cout << g.name << ", ";
        }
        std::cout << "\n";
        (void)c;
    }

} // namespace

int main(int argc, char **argv)
{
    const std::string path = (argc > 1) ? argv[1] : "fixtures/closet.json";

    const std::vector<engine::Garment> closet = engine::load_closet(path);
    if (closet.empty())
    {
        std::cerr << "no garments loaded\n";
        return 1;
    }
    std::cout << "closet: " << closet.size() << " garments\n\n";

    for (const engine::Occasion &occasion : {engine::occasions::wedding(),
                                             engine::occasions::city_day_out(),
                                             engine::occasions::workday()})
    {
        const engine::Candidates c = engine::filter_closet(closet, occasion);
        std::cout << occasion.name << " -- " << c.total() << " of "
                  << closet.size() << " garments pass"
                  << (c.complete() ? "" : "   [INCOMPLETE: no outfit possible]")
                  << "\n";
        show(c, "tops   ", c.tops);
        show(c, "bottoms", c.bottoms);
        show(c, "shoes  ", c.shoes);
        std::cout << "\n";
    }
    return 0;
}

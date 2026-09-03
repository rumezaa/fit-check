#include <engine/filter.hpp>
#include <engine/garment.hpp>
#include <engine/loader.hpp>

#include <iostream>

namespace
{

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

} // namespace

int main(int argc, char **argv)
{
    const std::string closetPath = (argc > 1) ? argv[1] : "fixtures/closet.json";
    const std::string occasionPath = (argc > 2) ? argv[2] : "fixtures/occasions.json";

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

    std::cout << "closet: " << closet.size() << " garments, "
              << occasions.size() << " occasions\n\n";

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
        std::cout << "\n";
    }
    return 0;
}

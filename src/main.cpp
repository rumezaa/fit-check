#include <engine/garment.hpp>
#include <engine/loader.hpp>

#include <iostream>

int main(int argc, char **argv)
{
    const std::string path = (argc > 1) ? argv[1] : "fixtures/closet.json";

    const std::vector<engine::Garment> closet = engine::load_closet(path);
    if (closet.empty())
    {
        std::cerr << "no garments loaded\n";
        return 1;
    }

    std::cout << "loaded " << closet.size() << " garments\n\n";
    for (const engine::Garment &g : closet)
    {
        std::cout << g.id << "  " << g.name << "  [" << g.hex << "]"
                  << (g.clean ? "" : "  (dirty)");
        if (g.last_worn)
        {
            std::cout << "  worn " << engine::to_iso_date(*g.last_worn);
        }
        else
        {
            std::cout << "  never worn";
        }
        std::cout << "\n";
    }
    return 0;
}

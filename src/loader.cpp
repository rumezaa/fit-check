#include <engine/loader.hpp>

#include <nlohmann/json.hpp>

#include <array>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

using json = nlohmann::json;

namespace engine
{

    // Lch is a plain aggregate of floats whose member names match the JSON
    // keys exactly, which is the case this macro is built for.
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Lch, l, c, h)

    namespace
    {

        // looks a string up in a name -> value table.
        template <typename E, std::size_t N>
        E enumFromString(const std::string &text,
                         const std::array<std::pair<const char *, E>, N> &table,
                         const char *field)
        {
            for (const auto &[name, value] : table)
            {
                if (text == name)
                {
                    return value;
                }
            }
            throw std::runtime_error(std::format("unknown {}: '{}'", field, text));
        }

        // these are the tables we made - faster lookup time than using a simple hash
        constexpr std::array kCategories{
            std::pair{"Top", Category::Top},
            std::pair{"Bottom", Category::Bottom},
            std::pair{"Shoes", Category::Shoes},
        };

        constexpr std::array kPatterns{
            std::pair{"Solid", Pattern::Solid},
            std::pair{"Floral", Pattern::Floral},
            std::pair{"PolkaDot", Pattern::PolkaDot},
            std::pair{"Stripes", Pattern::Stripes},
            std::pair{"Graphic", Pattern::Graphic},
        };

        constexpr std::array kFormalities{
            std::pair{"Casual", Formality::Casual},
            std::pair{"Business", Formality::Business},
            std::pair{"Elegant", Formality::Elegant},
        };

        std::chrono::sys_days parseIsoDate(const std::string &text)
        {

            // safety checks
            if (text.size() != 10)
            {
                throw std::runtime_error(
                    std::format("date is incorrect length '{}'", text));
            }

            if (text[4] != '-' || text[7] != '-')
            {
                throw std::runtime_error(
                    std::format("date is incorrect format '{}'", text));
            }

            auto isDigit = [](char c)
            { return c >= '0' && c <= '9'; };

            for (std::size_t i = 0; i < text.size(); i++)
            {
                if (i == 4 || i == 7)
                {
                    continue;
                }

                if (!isDigit(text[i]))
                {
                    throw std::runtime_error(
                        std::format("non digit present in date '{}'", text));
                }
            }

            // every character is known to be a digit now, so turn them into
            // numbers. '7' - '0' == 7, because the digits are adjacent in ASCII.
            const int year = (text[0] - '0') * 1000 + (text[1] - '0') * 100 +
                             (text[2] - '0') * 10 + (text[3] - '0');
            const int month = (text[5] - '0') * 10 + (text[6] - '0');
            const int day = (text[8] - '0') * 10 + (text[9] - '0');

            const std::chrono::year_month_day date{
                std::chrono::year{year},
                std::chrono::month{static_cast<unsigned>(month)},
                std::chrono::day{static_cast<unsigned>(day)}};

            // Digits alone don't make a real date: 2026-02-30 gets here.
            if (!date.ok())
            {
                throw std::runtime_error(std::format("not a real date: '{}'", text));
            }

            return std::chrono::sys_days{date};
        }

    } // namespace

    std::string to_iso_date(std::chrono::sys_days day)
    {
        return std::format("{:%F}", day);
    }

    std::vector<Garment> load_closet(const std::filesystem::path &path)
    {
        // load the file
        std::ifstream inputFile(path);

        if (!inputFile.is_open())
        {
            std::cerr << "Error opening file: " << path << std::endl;
            return {};
        }

        std::vector<Garment> garmentVector;

        try
        {
            json closetData;
            // now hold our entire document
            inputFile >> closetData;

            // closetData is the top-level object; the garments live under a key.
            const json &garments = closetData.at("garments");
            garmentVector.reserve(garments.size());

            for (const auto &item : garments)
            {
                Garment g;
                g.id = item.at("id").get<int>();
                g.name = item.at("name").get<std::string>();
                g.category = enumFromString(item.at("category").get<std::string>(),
                                            kCategories, "category");
                g.color = item.at("color").get<Lch>();
                g.hex = item.at("hex").get<std::string>();
                g.pattern = enumFromString(item.at("pattern").get<std::string>(),
                                           kPatterns, "pattern");
                g.formality = enumFromString(item.at("formality").get<std::string>(),
                                             kFormalities, "formality");
                g.clean = item.at("clean").get<bool>();
                g.weight = item.at("weight").get<float>();
                g.warmth = item.at("warmth").get<int>();

                // null means never worn, which is an empty optional.
                const json &worn = item.at("last_worn");
                if (!worn.is_null())
                {
                    g.last_worn = parseIsoDate(worn.get<std::string>());
                }

                garmentVector.push_back(std::move(g));
            }
        }
        catch (const std::exception &e)
        {
            // Covers json::parse_error (bad JSON), json::out_of_range (missing
            // key) and the runtime_errors thrown above.
            std::cerr << "Error reading " << path << ": " << e.what() << std::endl;
            return {};
        }

        return garmentVector;
    }

    std::vector<Occasion> load_occasions(const std::filesystem::path &path)
    {
        std::ifstream inputFile(path);

        if (!inputFile.is_open())
        {
            std::cerr << "Error opening file: " << path << std::endl;
            return {};
        }

        std::vector<Occasion> occasionVector;

        try
        {
            json occasionData;
            inputFile >> occasionData;

            const json &entries = occasionData.at("occasions");
            occasionVector.reserve(entries.size());

            for (const auto &item : entries)
            {
                Occasion o;
                o.name = item.at("name").get<std::string>();
                o.min_formality = enumFromString(
                    item.at("min_formality").get<std::string>(), kFormalities,
                    "min_formality");
                o.max_formality = enumFromString(
                    item.at("max_formality").get<std::string>(), kFormalities,
                    "max_formality");

                o.require_clean = item.value("require_clean", o.require_clean);

                if (o.min_formality > o.max_formality)
                {
                    throw std::runtime_error(std::format(
                        "occasion '{}' has min_formality above max_formality",
                        o.name));
                }

                occasionVector.push_back(std::move(o));
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error reading " << path << ": " << e.what() << std::endl;
            return {};
        }

        return occasionVector;
    }

}

#include <engine/loader.hpp>
#include <engine/request.hpp>

#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
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

        constexpr std::array kSilhouettes{
            std::pair{"Fitted", Silhouette::Fitted},
            std::pair{"Straight", Silhouette::Straight},
            std::pair{"Flowy", Silhouette::Flowy},
            std::pair{"Oversized", Silhouette::Oversized},
        };

        constexpr std::array kLengths{
            std::pair{"NA", Length::NA},
            std::pair{"Mini", Length::Mini},
            std::pair{"Midi", Length::Midi},
            std::pair{"Maxi", Length::Maxi},
        };

        constexpr std::array kFabrics{
            std::pair{"Cotton", Fabric::Cotton},
            std::pair{"Denim", Fabric::Denim},
            std::pair{"Knit", Fabric::Knit},
            std::pair{"Linen", Fabric::Linen},
            std::pair{"Wool", Fabric::Wool},
            std::pair{"Silk", Fabric::Silk},
            std::pair{"Satin", Fabric::Satin},
            std::pair{"Lace", Fabric::Lace},
            std::pair{"Leather", Fabric::Leather},
        };

        // reads a list of names into enum values - a missing key gives us an
        // empty vector which everything reads as no preference
        template <typename E, std::size_t N>
        std::vector<E> enumListFromJson(const json &parent, const char *key,
                                        const std::array<std::pair<const char *, E>, N> &table)
        {
            std::vector<E> values;
            if (!parent.contains(key))
            {
                return values;
            }

            const json &entries = parent.at(key);
            values.reserve(entries.size());
            for (const auto &entry : entries)
            {
                values.push_back(
                    enumFromString(entry.get<std::string>(), table, key));
            }
            return values;
        }

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


        // the parsing itself, split out so a file and stdin can share it. these
        // throw instead of returning empty - whoever calls decides what to do
        std::vector<Garment> parseGarments(const json &garments)
        {
        std::vector<Garment> garmentVector;
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

            // optional so an older closet.json still loads, defaults sit on Garment
            g.silhouette = enumFromString(
                item.value("silhouette", std::string{"Straight"}), kSilhouettes,
                "silhouette");
            g.length = enumFromString(item.value("length", std::string{"NA"}),
                                      kLengths, "length");
            g.fabric = enumFromString(item.value("fabric", std::string{"Cotton"}),
                                      kFabrics, "fabric");
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
            return garmentVector;
        }

        Occasion parseOccasion(const json &item)
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

            return o;
        }

        std::vector<Occasion> parseOccasions(const json &entries)
        {
            std::vector<Occasion> occasionVector;
            occasionVector.reserve(entries.size());

            for (const auto &item : entries)
            {
                occasionVector.push_back(parseOccasion(item));
            }
            return occasionVector;
        }

        Aesthetic parseAesthetic(const json &item)
        {
        Aesthetic a;
        a.name = item.at("name").get<std::string>();

        // all optional - vibes only list the axes they care about so a
        // missing key keeps whatever default is on the struct
        const json weights = item.value("weights", json::object());
        a.weights.color = weights.value("color", a.weights.color);
        a.weights.formality = weights.value("formality", a.weights.formality);
        a.weights.pattern = weights.value("pattern", a.weights.pattern);
        a.weights.palette = weights.value("palette", a.weights.palette);
        a.weights.recency = weights.value("recency", a.weights.recency);
        a.weights.shape = weights.value("shape", a.weights.shape);

        const json palette = item.value("palette", json::object());
        a.palette.chroma_min = palette.value("chroma_min", a.palette.chroma_min);
        a.palette.chroma_max = palette.value("chroma_max", a.palette.chroma_max);
        a.palette.l_min = palette.value("l_min", a.palette.l_min);
        a.palette.l_max = palette.value("l_max", a.palette.l_max);
        a.palette.hue_center = palette.value("hue_center", a.palette.hue_center);
        a.palette.hue_spread = palette.value("hue_spread", a.palette.hue_spread);

        if (a.palette.chroma_min > a.palette.chroma_max)
        {
            throw std::runtime_error(std::format(
                "aesthetic '{}' has chroma_min above chroma_max", a.name));
        }

        if (a.palette.l_min > a.palette.l_max)
        {
            throw std::runtime_error(std::format(
                "aesthetic '{}' has l_min above l_max", a.name));
        }

        // hue_dist never goes past 180 so anything wider is probably a typo
        if (a.palette.hue_spread < 0.0f || a.palette.hue_spread > 180.0f)
        {
            throw std::runtime_error(std::format(
                "aesthetic '{}' has hue_spread {} outside 0-180",
                a.name, a.palette.hue_spread));
        }

        const json shape = item.value("shape", json::object());
        a.shape.silhouettes =
            enumListFromJson(shape, "silhouettes", kSilhouettes);
        a.shape.lengths = enumListFromJson(shape, "lengths", kLengths);
        a.shape.fabrics = enumListFromJson(shape, "fabrics", kFabrics);

        // the wheel wraps so 400 and -320 are both just 40
        a.palette.hue_center = std::fmod(a.palette.hue_center, 360.0f);
        if (a.palette.hue_center < 0.0f)
        {
            a.palette.hue_center += 360.0f;
        }

            return a;
        }

        std::vector<Aesthetic> parseAesthetics(const json &entries)
        {
            std::vector<Aesthetic> aestheticVector;
            aestheticVector.reserve(entries.size());

            for (const auto &item : entries)
            {
                aestheticVector.push_back(parseAesthetic(item));
            }
            return aestheticVector;
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

            garmentVector = parseGarments(closetData.at("garments"));
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

            occasionVector = parseOccasions(occasionData.at("occasions"));
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error reading " << path << ": " << e.what() << std::endl;
            return {};
        }

        return occasionVector;
    }

    std::vector<Aesthetic> load_aesthetics(const std::filesystem::path &path)
    {
        std::ifstream inputFile(path);

        if (!inputFile.is_open())
        {
            std::cerr << "Error opening file: " << path << std::endl;
            return {};
        }

        std::vector<Aesthetic> aestheticVector;

        try
        {
            json aestheticData;
            inputFile >> aestheticData;

            aestheticVector = parseAesthetics(aestheticData.at("aesthetics"));
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error reading " << path << ": " << e.what() << std::endl;
            return {};
        }

        return aestheticVector;
    }

    Request read_request(std::istream &in)
    {
        json doc;
        in >> doc;

        Request req;

        // one of each, already chosen by whoever is calling us - we used to
        // take catalogs and a name to look up, which just undid the lookup
        // they had already done
        //
        // both are optional now. asking what goes with one piece means naming
        // no occasion and no vibe, and the open ended pair below is what that
        // question scores against
        const auto has = [&doc](const char *key)
        { return doc.contains(key) && !doc.at(key).is_null(); };

        req.occasion = has("occasion") ? parseOccasion(doc.at("occasion"))
                                       : any_occasion();
        req.vibe = has("vibe") ? parseAesthetic(doc.at("vibe")) : any_vibe();

        if (has("anchor_id"))
        {
            req.anchor_id = doc.at("anchor_id").get<int>();
        }

        const json weather = doc.value("weather", json::object());
        req.weather.temp_c = weather.value("temp_c", req.weather.temp_c);

        if (has("seed"))
        {
            req.seed = doc.at("seed").get<std::uint32_t>();
        }
        req.limit = doc.value("limit", req.limit);

        req.closet = parseGarments(doc.at("closet"));

        return req;
    }

}

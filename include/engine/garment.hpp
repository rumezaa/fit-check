#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace engine
{

    enum class Pattern
    {
        Solid,
        Floral,
        PolkaDot,
        Stripes,
    };

    enum class Category
    {
        Top,
        Bottom,
        Shoes,
    };

    enum class Formality
    {
        Casual,
        Business,
        Elegant,
    };

    // perceptual color, this scale will be better to help us get colors that coordinate better together
    struct Lch
    {
        float l = 0; // lightness, 0-100
        float c = 0; // chroma (saturation
        float h = 0; // hue angle in degree
    };

    struct Garment
    {
        int id = 0;
        std::string name;
        Category category = Category::Top;

        Lch color;
        std::string hex; // "#1b2a5e", for display only; derive it from `color`
        Pattern pattern = Pattern::Solid;
        Formality formality = Formality::Casual;

        bool clean = true;
        float weight = 1.0; // multiplier for if the user wants to wear the piece more often
        int warmth;         // scale of 1-5 for the warmth of the garment

        // empty means never worn.
        std::optional<std::chrono::sys_days> last_worn;
    };

} // namespace engine

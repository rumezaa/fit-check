#pragma once
#include <string>

namespace engine
{

    struct PaletteTarget
    {
        float chroma_min = 0.0f, chroma_max = 100.0f;
        float l_min = 0.0f, l_max = 100.0f;
        float hue_center = 0.0f, hue_spread = 180.0f;
    };

    struct Weights
    {
        float color = 1.0f;
        float formality = 1.0f;
        float pattern = 1.0f;
        float palette = 0.0f;
    };

    struct Aesthetic
    {
        std::string name;
        Weights weights;
        PaletteTarget palette;
    };
}
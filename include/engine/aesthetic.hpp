#pragma once

#include <engine/garment.hpp>

#include <string>
#include <vector>

namespace engine
{

    struct PaletteTarget
    {
        float chroma_min = 0.0f, chroma_max = 100.0f;
        float l_min = 0.0f, l_max = 100.0f;
        float hue_center = 0.0f, hue_spread = 180.0f;
    };

    // what shape the vibe wants - an empty list means it has no opinion so we score neutral
    struct ShapeTarget
    {
        std::vector<Silhouette> silhouettes;
        std::vector<Length> lengths;
        std::vector<Fabric> fabrics;
    };

    struct Weights
    {
        float color = 1.0f;
        float formality = 1.0f;
        float pattern = 1.0f;
        float palette = 0.0f;
        float recency = 1.0f; // how much a vibe cares about not repeating
        float shape = 1.0f;   // silhouette, length and fabric together
    };

    struct Aesthetic
    {
        std::string name;
        Weights weights;
        PaletteTarget palette;
        ShapeTarget shape;
    };
}
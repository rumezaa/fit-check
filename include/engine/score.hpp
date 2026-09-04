#pragma once

#include <engine/filter.hpp>
#include <engine/garment.hpp>

namespace engine
{
    struct AestheticWeights
    {
        float color = 1.0f;
        float formality = 1.0f;
        float pattern = 1.0f;
    };

    struct ScoreRes
    {
        float color;
        float formality;
        float pattern;
        float total(const AestheticWeights &w) const; // weighted score sum
    };

    ScoreRes
    score_pair(const Garment &top, const Garment &bottom, const Occasion &occ);
}

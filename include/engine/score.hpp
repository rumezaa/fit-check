#pragma once

#include <engine/filter.hpp>
#include <engine/garment.hpp>
#include <engine/aesthetic.hpp>

#include <chrono>

namespace engine
{

    // every axis is on roughly the same scale so they stay comparable before the weights tip it
    struct ScoreRes
    {
        float color = 0.0f;
        float formality = 0.0f;
        float pattern = 0.0f;
        float palette = 0.0f;
        float recency = 0.0f;
        float shape = 0.0f;
        // the weights we scored with - palette was measured against this vibes
        // target so another vibes weights wouldnt mean anything
        Weights weights;

        float total() const; // weighted score sum
    };

    // whole vibe goes in not just the palette since total() needs the weights anyway
    // we pass today in instead of reading the clock so this stays deterministic
    ScoreRes score_pair(const Garment &top, const Garment &bottom,
                        const Occasion &occ, const Aesthetic &vibe,
                        std::chrono::sys_days today);
}

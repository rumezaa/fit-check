#pragma once

#include <engine/filter.hpp>
#include <engine/garment.hpp>
#include <engine/aesthetic.hpp>

namespace engine
{

    // Each axis is scored independently on roughly the same scale, so they
    // stay comparable before the aesthetic's weights tip the balance.
    struct ScoreRes
    {
        float color = 0.0f;
        float formality = 0.0f;
        float pattern = 0.0f;
        float palette = 0.0f;
        // The weights the vibe scored this with, kept so total() can't be
        // asked for a sum against some other vibe. The palette axis was
        // measured against this vibe's PaletteTarget, so mixing in another
        // vibe's weights would be meaningless anyway.
        Weights weights;

        float total() const; // weighted score sum
    };

    // The whole Aesthetic goes in, not just its PaletteTarget: ScoreRes::total
    // needs vibe.weights anyway, and passing them together means the palette
    // and the weights it was scored against can't drift apart.
    ScoreRes score_pair(const Garment &top, const Garment &bottom,
                        const Occasion &occ, const Aesthetic &vibe);
}

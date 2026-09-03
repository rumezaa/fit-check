#pragma once

#include <engine/garment.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace engine
{

    struct Occasion
    {
        std::string name;
        Formality min_formality = Formality::Casual;
        Formality max_formality = Formality::Elegant;

        bool require_clean = true;
    };

    // gets garments that match the occasion
    bool matches(const Garment &garment, const Occasion &occasion);

    struct Candidates
    {
        std::vector<Garment> tops;
        std::vector<Garment> bottoms;
        std::vector<Garment> shoes;

        // no outfit is possible unless all three are non-empty.
        bool complete() const;
        std::size_t total() const;
    };

    Candidates filter_closet(const std::vector<Garment> &closet,
                             const Occasion &occasion);

}

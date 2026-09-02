#pragma once

#include <engine/garment.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace engine
{

    // What the user picks: "Wedding", "Day out in the city".
    //
    // An occasion is a RULE for narrowing the closet, not a property of any
    // garment. That is why it lives here and not in garment.hpp.
    struct Occasion
    {
        std::string name;

        // Inclusive range. Formality is an ordered enum, so these compare
        // directly with < and >.
        Formality min_formality = Formality::Casual;
        Formality max_formality = Formality::Elegant;

        // Inclusive range on the 1-5 warmth scale.
        int min_warmth = 1;
        int max_warmth = 5;

        bool require_clean = true;
    };

    namespace occasions
    {
        Occasion wedding();
        Occasion city_day_out();
        Occasion workday();
    } // namespace occasions

    // True if this one garment satisfies the occasion's rules. Says nothing
    // about category -- a top and a shoe are judged the same way.
    bool matches(const Garment &garment, const Occasion &occasion);

    // Garments that survived the filter, split by category because that is the
    // shape the combination stage needs to pair them.
    //
    // Holds copies rather than pointers into the closet: a vector reallocates
    // when it grows, which would leave pointers dangling.
    struct Candidates
    {
        std::vector<Garment> tops;
        std::vector<Garment> bottoms;
        std::vector<Garment> shoes;

        // No outfit is possible unless all three are non-empty.
        bool complete() const;
        std::size_t total() const;
    };

    Candidates filter_closet(const std::vector<Garment> &closet,
                             const Occasion &occasion);

} // namespace engine

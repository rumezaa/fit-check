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

    // warmth lives here not on Occasion - the same picnic wants linen in july and a parka in january
    struct Weather
    {
        float temp_c = 18.0f;
    };

    // the warmth values we can wear at some temp - a band not one number so theres more than one right answer
    struct WarmthRange
    {
        int min = 1;
        int max = 5;
    };

    WarmthRange warmth_for(const Weather &weather);

    // gets garments that match the occasion in this weather
    bool matches(const Garment &garment, const Occasion &occasion,
                 const Weather &weather);

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
                             const Occasion &occasion, const Weather &weather);

}

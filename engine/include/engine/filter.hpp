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

    // what we filter against when the user didnt pick an occasion - it takes no
    // position on formality, so a piece is judged only against its partner
    inline Occasion any_occasion()
    {
        Occasion o;
        o.name = "Anything";
        return o;
    }

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

    // clean and warmth are the rules we never bend - a dirty shirt or a parka in
    // august isnt a worse outfit, its not an outfit
    bool wearable(const Garment &garment, const Occasion &occasion,
                  const Weather &weather);

    // formality is asked separately because we do bend it, when the closet
    // leaves us nothing else to reach for
    bool fits_formality(const Garment &garment, const Occasion &occasion);

    // gets garments that match the occasion in this weather
    bool matches(const Garment &garment, const Occasion &occasion,
                 const Weather &weather);

    // which rails we had to bend the dress code to fill - we hand this back so
    // a stretched outfit can be shown as a choice we made, not a mistake
    struct Relaxed
    {
        bool tops = false;
        bool bottoms = false;
        bool shoes = false;

        bool any() const;
    };

    struct Candidates
    {
        std::vector<Garment> tops;
        std::vector<Garment> bottoms;
        std::vector<Garment> shoes;

        Relaxed relaxed;

        // no outfit is possible unless all three are non-empty.
        bool complete() const;
        std::size_t total() const;
    };

    Candidates filter_closet(const std::vector<Garment> &closet,
                             const Occasion &occasion, const Weather &weather);

    // the piece the user is styling around, looked up in their closet. null
    // when nothing owns that id
    const Garment *find_garment(const std::vector<Garment> &closet, int id);

}

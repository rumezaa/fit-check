#include <engine/filter.hpp>

namespace engine
{

    WarmthRange warmth_for(const Weather &weather)
    {
        // bands overlap on purpose - 17 degrees shouldnt rule out what worked at 19
        if (weather.temp_c >= 25.0f)
        {
            return {1, 2};
        }
        if (weather.temp_c >= 18.0f)
        {
            return {1, 3};
        }
        if (weather.temp_c >= 10.0f)
        {
            return {2, 4};
        }
        if (weather.temp_c >= 2.0f)
        {
            return {3, 5};
        }
        return {4, 5};
    }

    bool matches(const Garment &garment, const Occasion &occasion,
                 const Weather &weather)
    {
        if (occasion.require_clean && !garment.clean)
        {
            return false;
        }

        // Formality is ordered, so a range check is just two comparisons.
        if (garment.formality < occasion.min_formality ||
            garment.formality > occasion.max_formality)
        {
            return false;
        }

        // hard filter not a score - a parka in august isnt a worse outfit, its not an outfit
        const WarmthRange warmth = warmth_for(weather);
        if (garment.warmth < warmth.min || garment.warmth > warmth.max)
        {
            return false;
        }

        return true;
    }

    // check if we have all aspects of our outfit ready
    bool Candidates::complete() const
    {
        return !tops.empty() && !bottoms.empty() && !shoes.empty();
    }

    std::size_t Candidates::total() const
    {
        return tops.size() + bottoms.size() + shoes.size();
    }

    // building possible combinations that pass our criteria
    Candidates filter_closet(const std::vector<Garment> &closet,
                             const Occasion &occasion, const Weather &weather)
    {
        Candidates candidates;

        for (const Garment &garment : closet)
        {
            if (!matches(garment, occasion, weather))
            {
                continue;
            }

            switch (garment.category)
            {
            case Category::Top:
                candidates.tops.push_back(garment);
                break;
            case Category::Bottom:
                candidates.bottoms.push_back(garment);
                break;
            case Category::Shoes:
                candidates.shoes.push_back(garment);
                break;
            }
        }

        return candidates;
    }

}

#include <engine/filter.hpp>

namespace engine
{

    bool matches(const Garment &garment, const Occasion &occasion)
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
                             const Occasion &occasion)
    {
        Candidates candidates;

        for (const Garment &garment : closet)
        {
            if (!matches(garment, occasion))
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

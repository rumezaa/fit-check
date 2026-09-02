#include <engine/filter.hpp>

namespace engine
{

    namespace occasions
    {

        Occasion wedding()
        {
            Occasion o;
            o.name = "Wedding";
            o.min_formality = Formality::Elegant;
            o.max_formality = Formality::Elegant;
            return o;
        }

        Occasion city_day_out()
        {
            Occasion o;
            o.name = "Day out in the city";
            o.min_formality = Formality::Casual;
            o.max_formality = Formality::Business;
            return o;
        }

        Occasion workday()
        {
            Occasion o;
            o.name = "Workday";
            o.min_formality = Formality::Business;
            o.max_formality = Formality::Elegant;
            return o;
        }

    } // namespace occasions

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

        if (garment.warmth < occasion.min_warmth ||
            garment.warmth > occasion.max_warmth)
        {
            return false;
        }

        return true;
    }

    bool Candidates::complete() const
    {
        return !tops.empty() && !bottoms.empty() && !shoes.empty();
    }

    std::size_t Candidates::total() const
    {
        return tops.size() + bottoms.size() + shoes.size();
    }

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

} // namespace engine

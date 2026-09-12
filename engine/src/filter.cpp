#include <engine/filter.hpp>

#include <array>

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

    bool wearable(const Garment &garment, const Occasion &occasion,
                  const Weather &weather)
    {
        if (occasion.require_clean && !garment.clean)
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

    int formality_rank(Formality formality)
    {
        switch (formality)
        {
        case Formality::Casual:
            return 0;
        case Formality::Business:
            return 1;
        case Formality::Elegant:
            return 2;
        }
        return 0;
    }

    int formality_gap(Formality formality, const Occasion &occasion)
    {
        const int rank = formality_rank(formality);
        const int lo = formality_rank(occasion.min_formality);
        const int hi = formality_rank(occasion.max_formality);

        if (rank < lo)
        {
            return lo - rank;
        }
        if (rank > hi)
        {
            return rank - hi;
        }
        return 0;
    }

    // one definition of "outside the band" - the filter and the score both
    // read it off the same gap, so they can never disagree about who fits
    bool fits_formality(const Garment &garment, const Occasion &occasion)
    {
        return formality_gap(garment.formality, occasion) == 0;
    }

    bool matches(const Garment &garment, const Occasion &occasion,
                 const Weather &weather)
    {
        return wearable(garment, occasion, weather) &&
               fits_formality(garment, occasion);
    }

    bool Relaxed::any() const
    {
        return tops || bottoms || shoes || anchor;
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

    namespace
    {
        // one rail of the closet, so the two passes below can walk all three
        // without writing the same switch twice
        struct Rail
        {
            Category category;
            std::vector<Garment> *items;
            bool *relaxed;
        };

        std::array<Rail, 3> rails_of(Candidates &candidates)
        {
            return {
                Rail{Category::Top, &candidates.tops, &candidates.relaxed.tops},
                Rail{Category::Bottom, &candidates.bottoms,
                     &candidates.relaxed.bottoms},
                Rail{Category::Shoes, &candidates.shoes,
                     &candidates.relaxed.shoes},
            };
        }
    } // namespace

    // building possible combinations that pass our criteria
    Candidates filter_closet(const std::vector<Garment> &closet,
                             const Occasion &occasion, const Weather &weather)
    {
        Candidates candidates;
        const std::array<Rail, 3> rails = rails_of(candidates);

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

        // an empty rail means the dress code asked for something we dont own.
        // date night wants business up, and if all we have is jeans then jeans
        // is the answer - we would rather hand back a stretched outfit than
        // nothing at all, so we refill that one rail ignoring formality and let
        // score_pair charge for the gap.
        //
        // only the empty rail bends. a closet with one business skirt in it
        // still gets the skirt, not the jeans sitting next to it
        for (const Rail &rail : rails)
        {
            if (!rail.items->empty())
            {
                continue;
            }

            for (const Garment &garment : closet)
            {
                if (garment.category != rail.category ||
                    !wearable(garment, occasion, weather))
                {
                    continue;
                }
                rail.items->push_back(garment);
            }

            // still empty means clean or warmth emptied it, not the dress code
            *rail.relaxed = !rail.items->empty();
        }

        return candidates;
    }

    const Garment *find_garment(const std::vector<Garment> &closet, int id)
    {
        for (const Garment &garment : closet)
        {
            if (garment.id == id)
            {
                return &garment;
            }
        }
        return nullptr;
    }

}

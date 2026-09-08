#pragma once

#include <engine/aesthetic.hpp>
#include <engine/filter.hpp>
#include <engine/garment.hpp>

#include <cstdint>
#include <istream>
#include <optional>
#include <string>
#include <vector>

namespace engine
{

    // one ask from whoever is driving us - everything the pipeline needs comes
    // in together so the binary has no hidden file dependencies
    struct Request
    {
        // both fall back to the open ended any_occasion / any_vibe when the
        // request leaves them out, which is what styling around one piece does
        Occasion occasion;
        Aesthetic vibe;
        Weather weather;

        // the piece the user wants styled around. empty means build a whole
        // outfit off the occasion and vibe, set means they asked what goes with
        // this one and every pair we hand back has to use it
        std::optional<int> anchor_id;

        // leave empty and we seed off the clock, set it and the same request
        // gives back the same outfit every time
        std::optional<std::uint32_t> seed;

        // how many ranked pairs to hand back alongside the pick
        std::size_t limit = 5;

        std::vector<Garment> closet;
    };

    // reads one request off a stream. throws on bad json or a missing key so
    // the caller can turn that into an error response
    Request read_request(std::istream &in);

} // namespace engine

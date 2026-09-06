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
        std::string occasion;
        std::string vibe;
        Weather weather;

        // leave empty and we seed off the clock, set it and the same request
        // gives back the same outfit every time
        std::optional<std::uint32_t> seed;

        // how many ranked pairs to hand back alongside the pick
        std::size_t limit = 5;

        std::vector<Garment> closet;
        std::vector<Occasion> occasions;
        std::vector<Aesthetic> vibes;
    };

    // reads one request off a stream. throws on bad json or a missing key so
    // the caller can turn that into an error response
    Request read_request(std::istream &in);

} // namespace engine

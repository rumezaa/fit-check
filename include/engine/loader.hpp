#pragma once

#include <engine/filter.hpp>
#include <engine/garment.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace engine
{
    std::vector<Garment> load_closet(const std::filesystem::path &path);

    // Reads the occasion definitions (see fixtures/occasions.json).
    //
    // min_warmth, max_warmth and require_clean are optional in the file and
    // fall back to the defaults on Occasion; name and the two formality
    // bounds are required. Same error behaviour as load_closet: reports on
    // stderr and returns an empty vector.
    std::vector<Occasion> load_occasions(const std::filesystem::path &path);

    std::string to_iso_date(std::chrono::sys_days day);

}

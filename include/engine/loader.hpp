#pragma once

#include <engine/filter.hpp>
#include <engine/garment.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace engine
{
    std::vector<Garment> load_closet(const std::filesystem::path &path);

    // Reads the occasion definitions (see fixtures/occasions.json)
    //
    // omit for now btw -> min_warmth, max_warmth and require_clean are optional in the file
    // fall back to the defaults on Occasion; name and the two formality
    std::vector<Occasion> load_occasions(const std::filesystem::path &path);

    std::string to_iso_date(std::chrono::sys_days day);

}

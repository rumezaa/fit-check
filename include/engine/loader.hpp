#pragma once

#include <engine/garment.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace engine
{
    std::vector<Garment> load_closet(const std::filesystem::path &path);

    std::string to_iso_date(std::chrono::sys_days day);

}

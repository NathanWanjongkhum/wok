#include "regions.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::vector<TextRegion> load_regions(const std::string &path)
{
    std::ifstream file(path);
    if (!file)
    {
        throw std::runtime_error("Failed to open regions file: " + path);
    }

    std::vector<TextRegion> regions;
    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }
        std::istringstream iss(line);
        TextRegion r;
        if (iss >> r.tag >> r.x >> r.y >> r.w >> r.h)
        {
            regions.push_back(r);
        }
    }

    if (regions.empty())
    {
        throw std::runtime_error("No regions loaded from: " + path);
    }

    return regions;
}

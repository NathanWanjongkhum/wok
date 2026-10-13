#pragma once
#include <string>
#include <vector>

// A named rectangle to watch, in coordinates relative to the target
// window's top-left corner. Named TextRegion (not Region) because X11's
// Xutil.h already typedefs `Region` as struct _XRegion*.
struct TextRegion
{
    std::string tag;
    int x, y, w, h;
};

// Loads whitespace-separated "tag x y w h" lines from a text file.
// Blank lines and lines starting with '#' are ignored.
// Throws std::runtime_error if the file can't be read or no regions parse.
std::vector<TextRegion> load_regions(const std::string &path);

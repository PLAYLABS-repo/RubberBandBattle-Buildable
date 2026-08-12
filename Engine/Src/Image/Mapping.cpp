
#include "Mapping.h"
#include "json.hpp"

#include <fstream>
#include <string>

using json = nlohmann::json;

// Strip "folder/subfolder/" prefix.
// Adobe Animate can export full paths, while animation lookups
// commonly use only the bare sprite name.
static std::string stripPath(const std::string& name)
{
    size_t slash = name.find_last_of("/\\");

    if (slash != std::string::npos)
        return name.substr(slash + 1);

    return name;
}

bool Absolut::Atlas::load(const char* path)
{
    std::ifstream file(path);

    if (!file.is_open())
        return false;

    json j;

    try
    {
        file >> j;
    }
    catch (...)
    {
        return false;
    }

    if (!j.contains("ATLAS"))
        return false;

    if (!j["ATLAS"].contains("SPRITES"))
        return false;

    for (auto& s : j["ATLAS"]["SPRITES"])
    {
        if (!s.contains("SPRITE"))
            continue;

        auto& sp = s["SPRITE"];

        Absolut::Frame f;

        f.x = sp.value("x", 0.0f);
        f.y = sp.value("y", 0.0f);
        f.w = sp.value("w", 0.0f);
        f.h = sp.value("h", 0.0f);

        std::string fullName =
            sp.value("name", std::string(""));

        if (fullName.empty())
            continue;

        // Store using the bare sprite name.
        std::string bareName = stripPath(fullName);

        frames[bareName] = f;

        // Also keep the original full path.
        if (fullName != bareName)
            frames[fullName] = f;
    }

    return true;
}

bool Absolut::Atlas::get(
    const std::string& name,
    Frame& out
) const
{
    // First try the exact name.
    auto it = frames.find(name);

    if (it != frames.end())
    {
        out = it->second;
        return true;
    }

    // If the caller supplied a path, try the bare name.
    std::string bareName = stripPath(name);

    it = frames.find(bareName);

    if (it != frames.end())
    {
        out = it->second;
        return true;
    }

    return false;

}
 void Absolut::FreeAtlas(Absolut::Atlas* atlas) {
         if (!atlas)
            return;
     delete atlas;
     }


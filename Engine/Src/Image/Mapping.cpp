#include "Mapping.h"
#include "json.hpp"

#include <fstream>
#include <string>

using json = nlohmann::json;


// ============================================================
// STRIP PATH
// ============================================================

static std::string stripPath(const std::string& name)
{
    size_t slash = name.find_last_of("/\\");

    if (slash != std::string::npos)
        return name.substr(slash + 1);

    return name;
}


// ============================================================
// LOAD ATLAS
//
// Supports:
//
// 1. Adobe Animate
//
//    {
//        "ATLAS": {
//            "SPRITES": [
//                {
//                    "SPRITE": {
//                        "name": "...",
//                        "x": ...,
//                        "y": ...,
//                        "w": ...,
//                        "h": ...
//                    }
//                }
//            ]
//        }
//    }
//
// 2. TexturePacker
//
//    {
//        "frames": {
//            "sprite.png": {
//                "frame": {
//                    "x": ...,
//                    "y": ...,
//                    "w": ...,
//                    "h": ...
//                },
//                "rotated": true/false,
//                ...
//            }
//        }
//    }
// ============================================================

bool Absolut::Atlas::load(const char* path)
{
    frames.clear();

    if (!path)
        return false;


    // ========================================================
    // OPEN FILE
    // ========================================================

    std::ifstream file(path);

    if (!file.is_open())
        return false;


    // ========================================================
    // PARSE JSON
    // ========================================================

    json j;

    try
    {
        file >> j;
    }
    catch (...)
    {
        return false;
    }


    // ========================================================
    // TEXTUREPACKER MODE
    //
    // TexturePacker JSON has a root "frames" object.
    // ========================================================

    if (j.contains("frames") &&
        j["frames"].is_object())
    {
        json& texturePackerFrames =
            j["frames"];


        for (auto it =
             texturePackerFrames.begin();
             it != texturePackerFrames.end();
             ++it)
        {
            std::string fullName =
                it.key();

            json& sprite =
                it.value();


            if (!sprite.contains("frame"))
                continue;

            if (!sprite["frame"].is_object())
                continue;


            json& fr =
                sprite["frame"];


            Absolut::Frame f;

            f.x =
                fr.value(
                    "x",
                    0.0f
                );

            f.y =
                fr.value(
                    "y",
                    0.0f
                );

            f.w =
                fr.value(
                    "w",
                    0.0f
                );

            f.h =
                fr.value(
                    "h",
                    0.0f
                );


            if (f.w <= 0.0f ||
                f.h <= 0.0f)
            {
                continue;
            }


            // ------------------------------------------------
            // Store bare filename.
            // ------------------------------------------------

            std::string bareName =
                stripPath(fullName);

            frames[bareName] =
                f;


            // ------------------------------------------------
            // Also store original path.
            // ------------------------------------------------

            if (fullName != bareName)
            {
                frames[fullName] =
                    f;
            }
        }


        return !frames.empty();
    }


    // ========================================================
    // ADOBE ANIMATE MODE
    //
    // Keep compatibility with your existing format.
    // ========================================================

    if (j.contains("ATLAS") &&
        j["ATLAS"].is_object())
    {
        if (!j["ATLAS"].contains("SPRITES"))
            return false;


        if (!j["ATLAS"]["SPRITES"].is_array())
            return false;


        for (auto& s :
             j["ATLAS"]["SPRITES"])
        {
            if (!s.contains("SPRITE"))
                continue;


            auto& sp =
                s["SPRITE"];


            Absolut::Frame f;


            f.x =
                sp.value(
                    "x",
                    0.0f
                );

            f.y =
                sp.value(
                    "y",
                    0.0f
                );

            f.w =
                sp.value(
                    "w",
                    0.0f
                );

            f.h =
                sp.value(
                    "h",
                    0.0f
                );


            std::string fullName =
                sp.value(
                    "name",
                    std::string("")
                );


            if (fullName.empty())
                continue;


            if (f.w <= 0.0f ||
                f.h <= 0.0f)
            {
                continue;
            }


            // ------------------------------------------------
            // Bare name.
            // ------------------------------------------------

            std::string bareName =
                stripPath(fullName);


            frames[bareName] =
                f;


            // ------------------------------------------------
            // Full path.
            // ------------------------------------------------

            if (fullName != bareName)
            {
                frames[fullName] =
                    f;
            }
        }


        return !frames.empty();
    }


    // ========================================================
    // UNKNOWN FORMAT
    // ========================================================

    return false;
}


// ============================================================
// GET FRAME
// ============================================================

bool Absolut::Atlas::get(
    const std::string& name,
    Frame& out
) const
{
    // --------------------------------------------------------
    // Exact match.
    // --------------------------------------------------------

    auto it =
        frames.find(name);

    if (it != frames.end())
    {
        out = it->second;
        return true;
    }


    // --------------------------------------------------------
    // Bare filename.
    // --------------------------------------------------------

    std::string bareName =
        stripPath(name);

    it =
        frames.find(bareName);

    if (it != frames.end())
    {
        out = it->second;
        return true;
    }


    return false;
}


// ============================================================
// FREE ATLAS
// ============================================================

void Absolut::FreeAtlas(
    Absolut::Atlas* atlas
)
{
    if (!atlas)
        return;

    delete atlas;
}

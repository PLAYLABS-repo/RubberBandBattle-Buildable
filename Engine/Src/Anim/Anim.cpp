#include "Anim/Anim.h"
#include "Image/Texture.h"
#include "Image/Mapping.h"
#include "Engine/GLES2Render/CameraTransform.h"
#include "Geom/Quad.h"

#include "Engine/dependencies/include.h"

#include <cmath>
#include <fstream>

using json = nlohmann::json;

namespace Absolut
{

// =====================================================
// HELPERS
// =====================================================

static float SafeFloat(
    const json& j,
    const char* key,
    float fallback = 0.0f
)
{
    if (!j.contains(key) || j[key].is_null())
        return fallback;

    try
    {
        return j[key].get<float>();
    }
    catch (...)
    {
        return fallback;
    }
}


static int SafeInt(
    const json& j,
    const char* key,
    int fallback = 0
)
{
    if (!j.contains(key) || j[key].is_null())
        return fallback;

    try
    {
        return j[key].get<int>();
    }
    catch (...)
    {
        return fallback;
    }
}


static std::string SafeString(
    const json& j,
    const char* key,
    const std::string& fallback = ""
)
{
    if (!j.contains(key) || j[key].is_null())
        return fallback;

    try
    {
        return j[key].get<std::string>();
    }
    catch (...)
    {
        return fallback;
    }
}


// =====================================================
// PARSE ELEMENT
// =====================================================
//
// Supported:
//
// 1. ATLAS_SPRITE_instance
// 2. SYMBOL_Instance containing bitmap
// 3. SYMBOL_Instance containing another symbol
//
// =====================================================

static AnimElement ParseElement(const json& e)
{
    AnimElement el;

    // =================================================
    // ATLAS SPRITE
    // =================================================

    if (e.contains("ATLAS_SPRITE_instance") &&
        !e["ATLAS_SPRITE_instance"].is_null())
    {
        const auto& sp =
            e["ATLAS_SPRITE_instance"];

        el.SpriteName =
            SafeString(sp, "name");

        if (sp.contains("Position") &&
            !sp["Position"].is_null())
        {
            el.Position.x =
                SafeFloat(
                    sp["Position"],
                    "x"
                );

            el.Position.y =
                SafeFloat(
                    sp["Position"],
                    "y"
                );
        }

        if (sp.contains("transformationPoint") &&
            !sp["transformationPoint"].is_null())
        {
            el.Pivot.x =
                SafeFloat(
                    sp["transformationPoint"],
                    "x"
                );

            el.Pivot.y =
                SafeFloat(
                    sp["transformationPoint"],
                    "y"
                );
        }
    }


    // =================================================
    // SYMBOL INSTANCE
    // =================================================

    if (e.contains("SYMBOL_Instance") &&
        !e["SYMBOL_Instance"].is_null())
    {
        const auto& inst =
            e["SYMBOL_Instance"];


        // =============================================
        // DECOMPOSED TRANSFORM
        // =============================================

        if (inst.contains("DecomposedMatrix") &&
            !inst["DecomposedMatrix"].is_null())
        {
            const auto& dm =
                inst["DecomposedMatrix"];


            // Position
            if (dm.contains("Position") &&
                !dm["Position"].is_null())
            {
                el.Position.x =
                    SafeFloat(
                        dm["Position"],
                        "x"
                    );

                el.Position.y =
                    SafeFloat(
                        dm["Position"],
                        "y"
                    );
            }


            // Scale
            if (dm.contains("Scaling") &&
                !dm["Scaling"].is_null())
            {
                el.Scale.x =
                    SafeFloat(
                        dm["Scaling"],
                        "x",
                        1.0f
                    );

                el.Scale.y =
                    SafeFloat(
                        dm["Scaling"],
                        "y",
                        1.0f
                    );
            }


            // Rotation
            if (dm.contains("Rotation") &&
                !dm["Rotation"].is_null())
            {
                el.Rotation =
                    SafeFloat(
                        dm["Rotation"],
                        "z"
                    );
            }
        }


        // =============================================
        // PIVOT
        // =============================================

        if (inst.contains("transformationPoint") &&
            !inst["transformationPoint"].is_null())
        {
            el.Pivot.x =
                SafeFloat(
                    inst["transformationPoint"],
                    "x"
                );

            el.Pivot.y =
                SafeFloat(
                    inst["transformationPoint"],
                    "y"
                );
        }


        // =============================================
        // BITMAP INSIDE SYMBOL
        // =============================================

        if (inst.contains("bitmap") &&
            !inst["bitmap"].is_null())
        {
            const auto& bm =
                inst["bitmap"];

            el.SpriteName =
                SafeString(
                    bm,
                    "name"
                );

            if (bm.contains("Position") &&
                !bm["Position"].is_null())
            {
                el.BitmapOff.x =
                    SafeFloat(
                        bm["Position"],
                        "x"
                    );

                el.BitmapOff.y =
                    SafeFloat(
                        bm["Position"],
                        "y"
                    );
            }
        }


        // =============================================
        // NESTED SYMBOL
        // =============================================

        else
        {
            el.SymbolName =
                SafeString(
                    inst,
                    "SYMBOL_name"
                );

            std::string symbolType =
                SafeString(
                    inst,
                    "symbolType"
                );


            // =========================================
            // GRAPHIC SYMBOL
            // =========================================

            if (symbolType == "graphic")
            {
                el.IsGraphic = true;

                el.FirstFrame =
                    SafeInt(
                        inst,
                        "firstFrame",
                        0
                    );

                std::string loop =
                    SafeString(
                        inst,
                        "loop",
                        "loop"
                    );

                el.Looping =
                    (loop == "loop");
            }
        }
    }

    return el;
}


// =====================================================
// PARSE LAYERS
// =====================================================

static int ParseLayers(
    const json& animNode,
    AnimTimeline& out
)
{
    int maxFrame = 0;

    if (!animNode.contains("LAYERS"))
        return 1;


    for (auto& layer : animNode["LAYERS"])
    {
        AnimLayer l;

        if (!layer.contains("Frames"))
            continue;


        for (auto& frame : layer["Frames"])
        {
            AnimFrame f;

            f.Index =
                SafeInt(
                    frame,
                    "index"
                );

            f.Duration =
                SafeInt(
                    frame,
                    "duration",
                    1
                );

            if (f.Duration < 1)
                f.Duration = 1;


            int end =
                f.Index +
                f.Duration;

            if (end > maxFrame)
                maxFrame = end;


            if (frame.contains("elements") &&
                frame["elements"].is_array())
            {
                for (auto& e :
                     frame["elements"])
                {
                    f.Elements.push_back(
                        ParseElement(e)
                    );
                }
            }


            l.Frames.push_back(
                std::move(f)
            );
        }


        out.Layers.push_back(
            std::move(l)
        );
    }


    return maxFrame > 0
        ? maxFrame
        : 1;
}


// =====================================================
// CONSTRUCTOR / DESTRUCTOR
// =====================================================

Animator::~Animator()
{
    FreeAnim();
}


// =====================================================
// FREE ANIMATION
// =====================================================
//
// ActiveAnim is a pointer into Symbols.
//
// Therefore:
//
// 1. Set ActiveAnim = nullptr
// 2. Clear Symbols
//
// Never delete ActiveAnim directly.
//
// =====================================================

void Animator::FreeAnim()
{
    ActiveAnim = nullptr;

    Symbols.clear();

    CurrentFrame = 0;
    TotalFrames = 0;

    FrameTimer = 0.0f;

    Looping = true;
    Finished = false;
}


// =====================================================
// GLOBAL FREE FUNCTION
// =====================================================

void FreeAnim(Animator& anim)
{
    anim.FreeAnim();
}


// =====================================================
// LOAD
// =====================================================

bool Animator::Load(const char* path)
{
    if (!path)
        return false;


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


    // =================================================
    // Validate the file before destroying the current
    // animation.
    // =================================================

    bool hasSymbols =
        j.contains("SYMBOL_DICTIONARY") &&
        j["SYMBOL_DICTIONARY"].contains("Symbols");

    bool hasRoot =
        j.contains("ANIMATION") &&
        !j["ANIMATION"].is_null();


    if (!hasSymbols && !hasRoot)
        return false;


    // =================================================
    // New animation data is built separately.
    // This prevents ActiveAnim from becoming invalid
    // while loading.
    // =================================================

    std::map<
        std::string,
        AnimTimeline
    > newSymbols;


    // =================================================
    // SYMBOL DICTIONARY
    // =================================================

    if (hasSymbols)
    {
        for (auto& sym :
             j["SYMBOL_DICTIONARY"]["Symbols"])
        {
            std::string symName =
                SafeString(
                    sym,
                    "SYMBOL_name"
                );

            if (symName.empty())
                continue;


            AnimTimeline t;


            if (sym.contains("TIMELINE") &&
                !sym["TIMELINE"].is_null())
            {
                t.TotalFrames =
                    ParseLayers(
                        sym["TIMELINE"],
                        t
                    );
            }
            else
            {
                t.TotalFrames = 1;
            }


            newSymbols[symName] =
                std::move(t);
        }
    }

    // =================================================
    // ROOT ANIMATION
    // =================================================

    std::string rootName =
        "ROOT_ANIM";

    bool foundRoot = false;


    if (hasRoot)
    {
        const auto& anim =
            j["ANIMATION"];


        rootName =
            SafeString(
                anim,
                "SYMBOL_name",
                "ROOT_ANIM"
            );


        if (anim.contains("TIMELINE") &&
            !anim["TIMELINE"].is_null())
        {
            AnimTimeline rootTimeline;


            rootTimeline.TotalFrames =
                ParseLayers(
                    anim["TIMELINE"],
                    rootTimeline
                );


            newSymbols[rootName] =
                std::move(rootTimeline);

            foundRoot = true;
        }
    }


    // =================================================
    // Make sure something was actually loaded.
    // =================================================

    if (newSymbols.empty())
        return false;


    // =================================================
    // Destroy old animation now.
    //
    // ActiveAnim MUST be invalidated before Symbols
    // is replaced.
    // =================================================

    ActiveAnim = nullptr;

    Symbols.clear();


    // =================================================
    // Move new animation data in.
    // =================================================

    Symbols =
        std::move(newSymbols);


    // =================================================
    // ROOT ANIMATION HAS PRIORITY
    // =================================================

    if (foundRoot)
    {
        auto it =
            Symbols.find(rootName);

        if (it != Symbols.end())
        {
            ActiveAnim =
                &it->second;

            TotalFrames =
                ActiveAnim->TotalFrames;
        }
    }


    // =================================================
    // FALLBACK
    //
    // If there was no root ANIMATION block, use the
    // first *_ANIM_* symbol.
    // =================================================

    if (!ActiveAnim)
    {
        for (auto it = Symbols.begin();
             it != Symbols.end();
             ++it)
        {
            if (it->first.find("_ANIM_")
                != std::string::npos)
            {
                ActiveAnim =
                    &it->second;

                TotalFrames =
                    ActiveAnim->TotalFrames;

                break;
            }
        }
    }


    // =================================================
    // If we loaded symbols but couldn't find an
    // animation, still leave the data loaded.
    // =================================================

    CurrentFrame = 0;
    FrameTimer = 0.0f;

    Looping = true;
    Finished = false;


    return true;
}


// =====================================================
// PLAY
// =====================================================

void Animator::Play(
    const std::string& entity,
    const std::string& animType
)
{
    std::string key =
        entity +
        "_ANIM_" +
        animType;


    auto it =
        Symbols.find(key);


    if (it == Symbols.end())
        return;


    ActiveAnim =
        &it->second;

    TotalFrames =
        ActiveAnim->TotalFrames;

    CurrentFrame = 0;

    FrameTimer = 0.0f;

    Looping = true;

    Finished = false;
}


// =====================================================
// PLAY LOOP
// =====================================================

void Animator::PlayLoopAnim(
    const std::string& entity,
    const std::string& animType
)
{
    Play(
        entity,
        animType
    );

    Looping = true;

    Finished = false;
}


// =====================================================
// PLAY ONCE
// =====================================================

void Animator::PlayOnce(
    const std::string& entity,
    const std::string& animType
)
{
    Play(
        entity,
        animType
    );

    Looping = false;

    Finished = false;
}


// =====================================================
// CHANGE PART
// =====================================================

void Animator::ChangePart(
    const std::string& oldSprite,
    const std::string& newSprite,
    const std::string& animKey
)
{
    auto it =
        Symbols.find(animKey);


    if (it == Symbols.end())
        return;


    SwapSpriteInAnim(
        it->second,
        oldSprite,
        newSprite,
        false
    );
}


// =====================================================
// CHANGE PARTS
// =====================================================

void Animator::ChangeParts(
    const std::string& oldSprite,
    const std::string& newSprite,
    const std::string& animKey
)
{
    auto it =
        Symbols.find(animKey);


    if (it == Symbols.end())
        return;


    SwapSpriteInAnim(
        it->second,
        oldSprite,
        newSprite,
        true
    );
}


// =====================================================
// SWAP SPRITE IN ANIMATION
// =====================================================

void Animator::SwapSpriteInAnim(
    AnimTimeline& timeline,
    const std::string& oldSprite,
    const std::string& newSprite,
    bool recursive
)
{
    for (auto& layer :
         timeline.Layers)
    {
        for (auto& frame :
             layer.Frames)
        {
            for (auto& el :
                 frame.Elements)
            {
                SwapSpriteInElement(
                    el,
                    oldSprite,
                    newSprite,
                    recursive
                );
            }
        }
    }
}


// =====================================================
// SWAP SPRITE IN ELEMENT
// =====================================================

void Animator::SwapSpriteInElement(
    AnimElement& el,
    const std::string& oldSprite,
    const std::string& newSprite,
    bool recursive
)
{
    if (el.SpriteName == oldSprite)
        el.SpriteName = newSprite;


    if (recursive &&
        !el.SymbolName.empty())
    {
        auto it =
            Symbols.find(
                el.SymbolName
            );


        if (it != Symbols.end())
        {
            SwapSpriteInAnim(
                it->second,
                oldSprite,
                newSprite,
                true
            );
        }
    }
}


// =====================================================
// UPDATE
// =====================================================

void Animator::Update(float dt)
{
    if (!ActiveAnim || Finished)
        return;

    if (Fps <= 0.0f)
        return;

    FrameTimer += dt;

    const float frameTime = 1.0f / Fps;

    while (FrameTimer >= frameTime)
    {
        FrameTimer -= frameTime;

        CurrentFrame++;

        if (CurrentFrame >= TotalFrames)
        {
            if (Looping)
            {
                CurrentFrame = 0;
            }
            else
            {
                CurrentFrame =
                    TotalFrames > 0
                    ? TotalFrames - 1
                    : 0;

                Finished = true;
                break;
            }
        }
    }
}


// =====================================================
// DRAW
// =====================================================

void Animator::Draw(
    Image* img,
    Atlas* atlas,
    Camera& cam
)
{
    (void)cam;

    if (!ActiveAnim)
        return;

    if (!img || !atlas)
        return;

    glEnable(GL_BLEND);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );

    glColor4f(
        1.0f,
        1.0f,
        1.0f,
        1.0f
    );

    Vec2 rootPos =
        Parent.Enabled
        ? Parent.Position
        : Vec2{0.0f, 0.0f};

    float rootRot =
        Parent.Enabled
        ? Parent.Rotation
        : 0.0f;

    Vec2 rootScale =
        Parent.Enabled
        ? Parent.Scale
        : Vec2{1.0f, 1.0f};

    DrawAnim(
        *ActiveAnim,
        img,
        atlas,
        rootPos,
        rootRot,
        rootScale,
        CurrentFrame
    );


}


// =====================================================
// SPRITE DRAWING
// =====================================================

void Animator::DrawSprite(
    const std::string& name,
    Image* img,
    Atlas* atlas,
    Vec2 pos,
    float rotRad,
    Vec2 scale,
    Vec2 pivot,
    Vec2 bitmapOff
)
{
    if (name.empty())
        return;

    if (!img || !atlas)
        return;

    Frame fr;

    if (!atlas->get(name, fr))
        return;

    Quad q;

    q.texture = img->Texture;

    q.x = pos.x + bitmapOff.x * scale.x;
    q.y = pos.y + bitmapOff.y * scale.y;

    q.w = fr.w * scale.x;
    q.h = fr.h * scale.y;

    q.PivotX = pivot.x;
    q.PivotY = pivot.y;

    q.Rotation = rotRad * (180.0f / 3.14159265f);

    if (img->width > 0 && img->height > 0)
    {
        q.u0 = fr.x / (float)img->width;
        q.v0 = fr.y / (float)img->height;
        q.u1 = (fr.x + fr.w) / (float)img->width;
        q.v1 = (fr.y + fr.h) / (float)img->height;
    }

    q.r = 1.0f;
    q.g = 1.0f;
    q.b = 1.0f;

    q.draw();
}


// =====================================================
// RECURSIVE DRAWING
// =====================================================
//
// Layers are iterated in REVERSE. Layer index 0 is the
// topmost layer in the timeline panel (meant to render
// in front), so it must be drawn LAST.
//
// =====================================================
// =========================
// CORE RENDER
// =========================

void Animator::DrawAnim(
    AnimTimeline& timeline,
    Image*        img,
    Atlas*        atlas,
    Vec2          parentPos,
    float         parentRot,
    Vec2          parentScale,
    int           frame
)
{
    if (timeline.TotalFrames <= 0)
        return;


    // =====================================================
    // DRAW BACK TO FRONT
    //
    // Animate/Flash layer 0 is the TOP layer.
    // Therefore we draw the highest layer index first
    // and layer 0 last.
    // =====================================================

    for (size_t layerIndex = timeline.Layers.size();
         layerIndex-- > 0;)
    {
        AnimLayer& layer =
            timeline.Layers[layerIndex];


        // =================================================
        // FIND THE FRAME THAT CONTAINS 'frame'
        // =================================================

        for (size_t frameIndex = 0;
             frameIndex < layer.Frames.size();
             ++frameIndex)
        {
            AnimFrame& animFrame =
                layer.Frames[frameIndex];


            int start =
                animFrame.Index;

            int end =
                animFrame.Index +
                animFrame.Duration;


            if (frame < start ||
                frame >= end)
            {
                continue;
            }


            // =================================================
            // DRAW ELEMENTS IN THIS FRAME
            // =================================================

            for (size_t elementIndex = 0;
                 elementIndex < animFrame.Elements.size();
                 ++elementIndex)
            {
                AnimElement& element =
                    animFrame.Elements[elementIndex];


                // =============================================
                // CALCULATE ELEMENT TRANSFORM
                // =============================================

                float cosR =
                    cosf(parentRot);

                float sinR =
                    sinf(parentRot);


                float localX =
                    element.Position.x *
                    parentScale.x;

                float localY =
                    element.Position.y *
                    parentScale.y;


              Vec2 worldPos(
    parentPos.x +
    cosR * localX -
    sinR * localY,

    parentPos.y +
    sinR * localX +
    cosR * localY
);


                float worldRot =
                    parentRot +
                    element.Rotation;


                Vec2 worldScale(
                    parentScale.x *
                    element.Scale.x,

                    parentScale.y *
                    element.Scale.y
                );


                // =============================================
                // DRAW SPRITE
                // =============================================

                if (!element.SpriteName.empty())
                {
                    DrawSprite(
                        element.SpriteName,

                        img,
                        atlas,

                        worldPos,

                        worldRot,

                        worldScale,

                        element.Pivot,

                        element.BitmapOff
                    );
                }


                // =============================================
                // DRAW NESTED SYMBOL
                // =============================================

                if (!element.SymbolName.empty())
                {
                    std::map<
                        std::string,
                        AnimTimeline
                    >::iterator it =
                        Symbols.find(
                            element.SymbolName
                        );


                    if (it != Symbols.end())
                    {
                        AnimTimeline& child =
                            it->second;


                        int childTotal =
                            child.TotalFrames;


                        if (childTotal <= 0)
                            childTotal = 1;


                        int childFrame;


                        // =====================================
                        // GRAPHIC SYMBOL
                        // =====================================

                        if (element.IsGraphic)
                        {
                            childFrame =
                                element.FirstFrame;


                            if (element.Looping)
                            {
                                childFrame %=
                                    childTotal;
                            }
                            else
                            {
                                if (childFrame >= childTotal)
                                {
                                    childFrame =
                                        childTotal - 1;
                                }
                            }
                        }

                        // =====================================
                        // NORMAL NESTED SYMBOL
                        // =====================================

                        else
                        {
                            childFrame =
                                frame;

                            childFrame %=
                                childTotal;
                        }


                        // =====================================
                        // RECURSE
                        // =====================================

                        DrawAnim(
                            child,

                            img,
                            atlas,

                            worldPos,

                            worldRot,

                            worldScale,

                            childFrame
                        );
                    }
                }
            }


            // =================================================
            // IMPORTANT:
            //
            // A layer can contain multiple keyframes.
            // Once the frame containing 'frame' has been
            // found, STOP searching this layer.
            // =================================================

            break;
        }
    }
}
} // namespace Absolut21`

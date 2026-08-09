/*******************************************************
filename: Animator.h

creation time: 9/08/2026
*******************************************************/

#pragma once

#include "Engine/dependencies/include.h"
#include "Math/Vec2.h"

namespace Absolut
{

class Image;
class Atlas;
class Camera;


// =====================================================
// ANIMATION ELEMENT
// =====================================================

struct AnimElement
{
    std::string SpriteName;
    std::string SymbolName;

    // Position of this element inside its parent symbol
    Vec2 Position = {0.0f, 0.0f};

    // Bitmap's own local offset inside the symbol
    Vec2 BitmapOff = {0.0f, 0.0f};

    // Element scale
    Vec2 Scale = {1.0f, 1.0f};

    // Rotation in radians
    float Rotation = 0.0f;

    // Flash/Animate transformation point
    Vec2 Pivot = {0.0f, 0.0f};


    // =================================================
    // GRAPHIC SYMBOL PLAYBACK
    // =================================================

    bool IsGraphic = false;

    int FirstFrame = 0;

    bool Looping = true;
};


// =====================================================
// ANIMATION FRAME
// =====================================================

struct AnimFrame
{
    int Index = 0;

    int Duration = 1;

    std::vector<AnimElement> Elements;
};


// =====================================================
// ANIMATION LAYER
// =====================================================

struct AnimLayer
{
    std::vector<AnimFrame> Frames;
};


// =====================================================
// ANIMATION TIMELINE
// =====================================================

struct AnimTimeline
{
    std::vector<AnimLayer> Layers;

    int TotalFrames = 0;
};


// =====================================================
// PARENT TRANSFORM
// =====================================================
//
// Describes a world-space anchor that this animator
// is parented to.
//
// Example:
//
// animator.Parent.Enabled  = true;
// animator.Parent.Position = {playerX, playerY};
// animator.Parent.Rotation = playerRotRad;
// animator.Parent.Scale    = {1, 1};
//
// =====================================================

struct AnimParentTransform
{
    bool Enabled = false;

    Vec2 Position = {0.0f, 0.0f};

    // Radians
    float Rotation = 0.0f;

    Vec2 Scale = {1.0f, 1.0f};
};


// =====================================================
// MAIN ANIMATOR CLASS
// =====================================================

class Animator
{
public:

    // =================================================
    // CONSTRUCTOR / DESTRUCTOR
    // =================================================

    Animator() = default;

    ~Animator();


    // =================================================
    // PARENT TRANSFORM
    // =================================================

    AnimParentTransform Parent;


    // =================================================
    // LOAD
    // =================================================

    bool Load(const char* path);


    // =================================================
    // FREE / DESTROY ANIMATION
    // =================================================
    //
    // Releases all loaded animation timelines.
    //
    // Safe to call manually.
    //
    // Load() should also call this before loading
    // another animation file.
    //
    // =================================================

    void FreeAnim();


    // =================================================
    // PLAY
    // =================================================

    // Plays:
    //
    // entity + "_ANIM_" + animType
    //
    // Example:
    //
    // Play("PLAYER", "RUN");
    //
    // -> PLAYER_ANIM_RUN
    //
    // Loops by default.

    void Play(
        const std::string& entity,
        const std::string& animType
    );


    // Explicit looping playback

    void PlayLoopAnim(
        const std::string& entity,
        const std::string& animType
    );


    // Plays once and stops on the last frame

    void PlayOnce(
        const std::string& entity,
        const std::string& animType
    );


    // =================================================
    // STATE
    // =================================================

    bool IsFinished() const
    {
        return Finished;
    }


    // =================================================
    // UPDATE / DRAW
    // =================================================

    void Update(float dt);

    void Draw(
        Image* img,
        Atlas* atlas,
        Camera& cam
    );


    // =================================================
    // PART SWAPPING
    // =================================================
    //
    // ChangePart:
    //     Only top-level timeline.
    //
    // ChangeParts:
    //     Top-level + recursive nested symbols.
    //
    // =================================================

    void ChangePart(
        const std::string& oldSprite,
        const std::string& newSprite,
        const std::string& animKey
    );


    void ChangeParts(
        const std::string& oldSprite,
        const std::string& newSprite,
        const std::string& animKey
    );


    // =================================================
    // PUBLIC ANIMATION STATE
    // =================================================

    int CurrentFrame = 0;

    int TotalFrames = 0;


private:

    // =================================================
    // LOADED SYMBOLS
    // =================================================

    std::map<
        std::string,
        AnimTimeline
    > Symbols;


    // =================================================
    // ACTIVE ANIMATION
    // =================================================

    AnimTimeline* ActiveAnim = nullptr;


    // =================================================
    // PLAYBACK STATE
    // =================================================

    float FrameTimer = 0.0f;

    float Fps = 30.0f;

    bool Looping = true;

    bool Finished = false;


    // =================================================
    // RECURSIVE DRAWING
    // =================================================

    void DrawAnim(
        AnimTimeline& timeline,
        Image* img,
        Atlas* atlas,

        Vec2 parentPos,

        float parentRot,

        Vec2 parentScale,

        int frame
    );


    // =================================================
    // SPRITE DRAWING
    // =================================================

    void DrawSprite(
        const std::string& name,

        Image* img,
        Atlas* atlas,

        Vec2 pos,

        float rotRad,

        Vec2 scale,

        Vec2 pivot,

        Vec2 bitmapOff
    );


    // =================================================
    // CHANGE PART HELPERS
    // =================================================

    void SwapSpriteInAnim(
        AnimTimeline& timeline,

        const std::string& oldSprite,
        const std::string& newSprite,

        bool recursive
    );


    void SwapSpriteInElement(
        AnimElement& el,

        const std::string& oldSprite,
        const std::string& newSprite,

        bool recursive
    );
};


// =====================================================
// GLOBAL FREE FUNCTION
// =====================================================
//
// Allows:
//
//     Absolut::FreeAnim(anim);
//
// This simply calls:
//
//     anim.FreeAnim();
//
// =====================================================

void FreeAnim(Animator& anim);

} // namespace Absolut

#pragma once

#include "Engine/dependencies/include.h"
#include "Engine/Src/Math/Vector.h"

namespace Absolut
{

// ============================================================
// COORDINATE SPACE
// ============================================================

enum QuadSpace
{
    WORLD,
    SCREEN
};


// ============================================================
// QUAD
// ============================================================

class Quad
{
public:

    // ========================================================
    // COLOR
    // ========================================================

  float r = 1.0f;
float g = 1.0f;
float b = 1.0f;
float a = 1.0f;


    // ========================================================
    // POSITION
    // ========================================================

    float x = 0.0f;
    float y = 0.0f;


    // ========================================================
    // SIZE
    // ========================================================

    float w = 0.0f;
    float h = 0.0f;


    // ========================================================
    // UV
    // ========================================================

    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;


    // ========================================================
    // TRANSFORM
    // ========================================================

    float SkewX = 0.0f;
    float SkewY = 0.0f;
    float Rotation = 0.0f;


    // ========================================================
    // PIVOT
    // ========================================================

    float PivotX = 0.0f;
    float PivotY = 0.0f;


    // ========================================================
    // BITMAP OFFSET
    // ========================================================

    float BitmapOffsetX = 0.0f;
    float BitmapOffsetY = 0.0f;


    // ========================================================
    // TEXTURE
    // ========================================================

    GLuint texture = 0;


private:

    // ========================================================
    // ANCHOR
    //
    // anchorFrom decides which projection Quad::draw() uses:
    //   WORLD  -> Absolut::ActiveProjection (camera applied)
    //   SCREEN -> fixed screen-space ortho (HUD/UI, ignores
    //             camera pan/zoom/rotation)
    //
    // anchorTo is stored for future use (e.g. converting a
    // quad's coordinates between spaces) but does not affect
    // rendering yet.
    // ========================================================

    QuadSpace anchorFrom = WORLD;
    QuadSpace anchorTo   = WORLD;

public:

    // ========================================================
    // ANCHOR TO
    // ========================================================

    void AnchorTo(
        QuadSpace from,
        QuadSpace to
    );


    // ========================================================
    // GETTERS
    // ========================================================

    QuadSpace GetAnchorFrom() const;
    QuadSpace GetAnchorTo() const;


    // ========================================================
    // DRAW
    // ========================================================

    void draw();
};

}

#pragma once

#include <cmath>
#include "Math/Vector.h"

namespace Absolut
{

// --------------------------------------------------------------
// ACTIVE PROJECTION
//
// GLES2 has no fixed-function matrix stack (no glMatrixMode,
// no GL_PROJECTION, no glOrtho, no glRotatef - those are
// desktop/GLES1 calls and are no-ops here, see Camera::apply
// below for what used to be here).
//
// Instead, this is the plain 4x4 matrix that Quad::draw() and
// Mesh::draw() upload into their uProjection uniform every draw
// call. It starts as identity so nothing crashes before the
// first Camera::apply() call.
//
// NOTE: 'inline' on a variable requires C++17. If your project
// isn't building with -std=c++17 / gnu++17, either enable that
// standard, or change this to 'extern float ActiveProjection[16];'
// here and give it exactly one definition in a single .cpp file.
// --------------------------------------------------------------

inline float ActiveProjection[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

enum class ProjectionMode
{
    Orthographic,
    Perspective
};

namespace CameraMath
{
    inline void Identity(float* m)
    {
        for (int i = 0; i < 16; ++i)
            m[i] = 0.0f;

        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    inline void Multiply(float* out, const float* a, const float* b)
    {
        float result[16];

        for (int col = 0; col < 4; ++col)
        {
            for (int row = 0; row < 4; ++row)
            {
                result[col * 4 + row] =
                    a[0 * 4 + row] * b[col * 4 + 0] +
                    a[1 * 4 + row] * b[col * 4 + 1] +
                    a[2 * 4 + row] * b[col * 4 + 2] +
                    a[3 * 4 + row] * b[col * 4 + 3];
            }
        }

        for (int i = 0; i < 16; ++i)
            out[i] = result[i];
    }

    // ------------------------------------------------------
    // PERSPECTIVE PROJECTION
    //
    // Standard right-handed GL-style perspective matrix,
    // column-major to match the storage convention used by
    // Multiply()/Identity() above (index = col * 4 + row).
    //
    // fovYDegrees: vertical field of view, in degrees.
    // aspect:      screenWidth / screenHeight.
    // nearZ/farZ:  MUST both be > 0, with farZ > nearZ.
    //              These are distances in front of the camera,
    //              not world-space Z like Camera::nearPlane /
    //              Camera::farPlane used for orthographic mode.
    // ------------------------------------------------------
    inline void Perspective(float* m, float fovYDegrees, float aspect, float nearZ, float farZ)
    {
        for (int i = 0; i < 16; ++i)
            m[i] = 0.0f;

        float fovRad = fovYDegrees * 3.14159265f / 180.0f;
        float f = 1.0f / tanf(fovRad * 0.5f);

        m[0]  = f / aspect;
        m[5]  = f;
        m[10] = (farZ + nearZ) / (nearZ - farZ);
        m[11] = -1.0f;
        m[14] = (2.0f * farZ * nearZ) / (nearZ - farZ);
    }
}

class Camera
{
public:

    // ------------------------------------------------------
    // POSITION
    //
    // Vec3: x/y are the usual 2D pan, z is the camera's
    // depth along the view axis. Objects (e.g. Mesh) placed
    // at different Z values are positioned relative to
    // wherever the camera currently sits on that axis.
    // ------------------------------------------------------

    Vec3 position = {0.0f, 0.0f, 0.0f};

    float zoom = 1.0f;
    float rotation = 0.0f; // degrees, roll around the view (Z) axis


    // ------------------------------------------------------
    // LOOK-AROUND (Perspective mode only)
    //
    // Separate from 'rotation' (roll) above. yaw turns the
    // camera left/right around world Y, pitch tilts it up/down
    // around its own local X axis. No roll is applied here -
    // 'rotation' is left completely independent of these.
    // ------------------------------------------------------

    float yaw   = 0.0f; // degrees, turn left/right (around world Y)
    float pitch = 0.0f; // degrees, look up/down (around local X)


    // ------------------------------------------------------
    // DEPTH RANGE (Orthographic mode)
    //
    // Relative to position.z. Anything outside
    // [position.z + nearPlane, position.z + farPlane] is
    // clipped. Tune to your scene's actual depth extents.
    // ------------------------------------------------------

    float nearPlane = -1000.0f;
    float farPlane  =  1000.0f;


    // ------------------------------------------------------
    // PROJECTION MODE
    //
    // Orthographic (default) preserves all prior behavior:
    // zoom scales the ortho half-extents, and position.x/y
    // pans by shifting left/right/top/bottom directly.
    //
    // Perspective uses fov/perspNear/perspFar instead, and
    // zoom is ignored (use position.z or fov to control the
    // apparent scale of the scene). position.x/y/z and yaw/
    // pitch become a real world-space camera transform rather
    // than a planar ortho shift.
    // ------------------------------------------------------

    ProjectionMode mode = ProjectionMode::Orthographic;

    float fov        = 60.0f;   // vertical FOV, degrees (Perspective only)
    float perspNear   = 1.0f;    // must be > 0 (Perspective only)
    float perspFar    = 1000.0f; // must be > perspNear (Perspective only)


    void apply(int screenWidth, int screenHeight);
};

inline void Camera::apply(int screenWidth, int screenHeight)
{
    // ----------------------------------------------------------
    // PROJECTION (orthographic or perspective, camera position
    // and zoom applied where applicable)
    // ----------------------------------------------------------

    float proj[16];

    if (mode == ProjectionMode::Perspective)
    {
        float aspect = (screenHeight != 0)
            ? (float)screenWidth / (float)screenHeight
            : 1.0f;

        CameraMath::Perspective(proj, fov, aspect, perspNear, perspFar);
    }
    else
    {
        float halfW = (screenWidth  * 0.5f) / zoom;
        float halfH = (screenHeight * 0.5f) / zoom;

        float left   = position.x - halfW;
        float right  = position.x + halfW;
        float top    = position.y - halfH;
        float bottom = position.y + halfH;

        // NOTE: intentionally NOT named 'near'/'far' - those are
        // legacy macros defined empty by Windows headers on
        // MinGW/MSVC and will break compilation if reused here.
        float nearZ = position.z + nearPlane;
        float farZ  = position.z + farPlane;

        CameraMath::Identity(proj);

        proj[0]  = 2.0f / (right - left);
        proj[5]  = 2.0f / (top - bottom);
        proj[10] = -2.0f / (farZ - nearZ);
        proj[12] = -(right + left) / (right - left);
        proj[13] = -(top + bottom) / (top - bottom);
        proj[14] = -(farZ + nearZ) / (farZ - nearZ);
    }

    // ----------------------------------------------------------
    // ROTATION (roll, around the view/screen-facing axis)
    //
    // Unchanged from before - still driven only by 'rotation',
    // independent of yaw/pitch below.
    // ----------------------------------------------------------

    float rot[16];

    CameraMath::Identity(rot);

    if (rotation != 0.0f)
    {
        float angle = rotation * 3.14159265f / 180.0f;
        float c = cosf(angle);
        float s = sinf(angle);

        rot[0] = c;  rot[1] = s;
        rot[4] = -s; rot[5] = c;
    }

    // ----------------------------------------------------------
    // VIEW (camera translation, plus yaw/pitch look-around in
    // Perspective mode - no roll enters this matrix)
    //
    // Orthographic mode keeps the original behavior: x/y pan is
    // already baked into the projection above via left/right/
    // top/bottom, so only Z is shifted here so that position.z
    // sits at the camera's own depth origin (lets objects with a
    // real Vec3 Z, e.g. Mesh, be positioned relative to the
    // camera's depth).
    //
    // Perspective mode has no left/right/top/bottom to pan with,
    // so the full position (x, y, z) plus yaw/pitch rotation is
    // applied here instead.
    // ----------------------------------------------------------

    float view[16];

    CameraMath::Identity(view);

    if (mode == ProjectionMode::Perspective)
    {
        float yawRad   = yaw   * 3.14159265f / 180.0f;
        float pitchRad = pitch * 3.14159265f / 180.0f;

        float cy = cosf(yawRad),   sy = sinf(yawRad);
        float cp = cosf(pitchRad), sp = sinf(pitchRad);

        // Yaw: rotation around world Y
        float rotView[16];
        CameraMath::Identity(rotView);
        rotView[0]  = cy;
        rotView[2]  = -sy;
        rotView[8]  = sy;
        rotView[10] = cy;

        // Pitch: rotation around local X
        float pitchMat[16];
        CameraMath::Identity(pitchMat);
        pitchMat[5]  = cp;
        pitchMat[6]  = sp;
        pitchMat[9]  = -sp;
        pitchMat[10] = cp;

        // Combined look-around rotation: pitch * yaw, no roll
        float combinedRot[16];
        CameraMath::Multiply(combinedRot, pitchMat, rotView);

        float translate[16];
        CameraMath::Identity(translate);
        translate[12] = -position.x;
        translate[13] = -position.y;
        translate[14] = -position.z;

        CameraMath::Multiply(view, combinedRot, translate);
    }
    else
    {
        view[14] = -position.z;
    }

    // ----------------------------------------------------------
    // Combine and publish for Quad::draw() / Mesh::draw() to
    // consume: ActiveProjection = proj * rot * view
    // ----------------------------------------------------------

    float rotated[16];

    CameraMath::Multiply(rotated, proj, rot);

    CameraMath::Multiply(ActiveProjection, rotated, view);
}

}

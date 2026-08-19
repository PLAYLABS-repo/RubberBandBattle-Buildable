#pragma once

#include "include.h"
#include "Mesh.h"
#include "Math/Vector.h"

namespace Absolut
{

class Box3
{
public:

    // ------------------------------------------------------------
    // TRANSFORM
    // ------------------------------------------------------------

    Vec3 position = {0.0f, 0.0f, 0.0f};

    Vec3 rotation = {0.0f, 0.0f, 0.0f};

    // ------------------------------------------------------------
    // SIZE
    // ------------------------------------------------------------

    float w = 1.0f;
    float h = 1.0f;
    float d = 1.0f;

    // ------------------------------------------------------------
    // APPEARANCE
    // ------------------------------------------------------------

    GLuint texture = 0;

    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;

    // ------------------------------------------------------------
    // RENDERING
    // ------------------------------------------------------------

    bool useLighting = true;
    bool enableCulling = true;

    bool visible = true;

    // ------------------------------------------------------------
    // INTERNAL MESH
    // ------------------------------------------------------------

private:

    Mesh mesh;

public:

    // ------------------------------------------------------------
    // CONSTRUCTORS
    // ------------------------------------------------------------

    Box3()
        : mesh(Mesh::CreateCube(1.0f))
    {
        UpdateMesh();
    }

    Box3(float width, float height, float depth)
        : w(width),
          h(height),
          d(depth),
          mesh(Mesh::CreateCube(1.0f))
    {
        UpdateMesh();
    }

    Box3(
        float width,
        float height,
        float depth,
        GLuint tex,
        float red,
        float green,
        float blue
    )
        : w(width),
          h(height),
          d(depth),
          texture(tex),
          r(red),
          g(green),
          b(blue),
          mesh(Mesh::CreateCube(1.0f))
    {
        UpdateMesh();
    }

    // ------------------------------------------------------------
    // UPDATE MESH
    // ------------------------------------------------------------

    void UpdateMesh()
    {
        mesh.position = position;

        mesh.rotation = rotation;

        mesh.scale = {
            w,
            h,
            d
        };

        mesh.texture = texture;

        mesh.r = r;
        mesh.g = g;
        mesh.b = b;

        mesh.useLighting = useLighting;

        mesh.enableCulling = enableCulling;
    }

    // ------------------------------------------------------------
    // DRAW
    // ------------------------------------------------------------

    void Draw()
    {
        if (!visible)
            return;

        UpdateMesh();

        mesh.draw();
    }

    // Lowercase version to match Mesh::draw()
    void draw()
    {
        Draw();
    }

    // ------------------------------------------------------------
    // POSITION
    // ------------------------------------------------------------

    void SetPosition(float x, float y, float z)
    {
        position.x = x;
        position.y = y;
        position.z = z;
    }

    void SetPosition(const Vec3& pos)
    {
        position = pos;
    }

    // ------------------------------------------------------------
    // ROTATION
    // ------------------------------------------------------------

    void SetRotation(float x, float y, float z)
    {
        rotation.x = x;
        rotation.y = y;
        rotation.z = z;
    }

    void SetRotation(const Vec3& rot)
    {
        rotation = rot;
    }

    // ------------------------------------------------------------
    // SIZE
    // ------------------------------------------------------------

    void SetSize(float width, float height, float depth)
    {
        w = width;
        h = height;
        d = depth;
    }

    // ------------------------------------------------------------
    // COLOR
    // ------------------------------------------------------------

    void SetColor(float red, float green, float blue)
    {
        r = red;
        g = green;
        b = blue;
    }

    // ------------------------------------------------------------
    // TEXTURE
    // ------------------------------------------------------------

    void SetTexture(GLuint tex)
    {
        texture = tex;
    }

    // ------------------------------------------------------------
    // LIGHTING
    // ------------------------------------------------------------

    void SetLighting(bool enabled)
    {
        useLighting = enabled;
    }

    // ------------------------------------------------------------
    // CULLING
    // ------------------------------------------------------------

    void SetCulling(bool enabled)
    {
        enableCulling = enabled;
    }

    // ------------------------------------------------------------
    // VISIBILITY
    // ------------------------------------------------------------

    void SetVisible(bool enabled)
    {
        visible = enabled;
    }

    bool IsVisible() const
    {
        return visible;
    }
};

}

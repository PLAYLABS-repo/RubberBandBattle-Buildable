#pragma once

#include <string>
#include <vector>
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "Engine/dependencies/include.h"

#include "Math/Vector.h"

namespace Absolut
{

// ============================================================
// MESH VERTEX
//
// Layout uploaded to the GPU for every Mesh, whether it came
// from a procedural generator (CreateCube/CreatePyramid/etc)
// or was loaded from a glTF file via LoadFromGLTF(). Matches
// the aPosition/aNormal/aTexCoord attributes bound in Mesh.cpp.
// ============================================================

struct MeshVertex
{
    float px, py, pz; // position
    float nx, ny, nz; // normal
    float u,  v;       // texcoord
};

class Mesh
{
public:

    Mesh() = default;
    ~Mesh();

    // Owns GL buffer objects - move-only, no implicit copies.
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // --------------------------------------------------------
    // PROCEDURAL PRIMITIVES
    //
    // All are centered on the local origin so position/rotate/
    // scale behave predictably. Sizes are in world units.
    // --------------------------------------------------------

    static Mesh CreateCube(float size = 1.0f);

    static Mesh CreatePyramid(
        float baseSize = 1.0f,
        float height = 1.0f
    );

    static Mesh CreateCylinder(
        float radius = 0.5f,
        float height = 1.0f,
        int segments = 24
    );

    static Mesh CreateSphere(
        float radius = 0.5f,
        int latSegments = 16,
        int lonSegments = 24
    );

    static Mesh CreatePlane(
        float width = 1.0f,
        float depth = 1.0f
    );

    // --------------------------------------------------------
    // GLTF LOADING (tinygltf)
    //
    // Loads the first primitive of the first mesh found in the
    // file. Picks .gltf (JSON+text) vs .glb (binary) based on
    // the file extension. Returns false and leaves this Mesh
    // untouched on failure - see stderr for tinygltf's error
    // string. Vertex indices are stored as unsigned short to
    // match GLES2's glDrawElements limits, so source meshes
    // over 65535 vertices will fail to load; split those in
    // your DCC/export step first.
    // --------------------------------------------------------

    bool LoadFromGLTF(const std::string& path);

    // --------------------------------------------------------
    // TRANSFORM
    // --------------------------------------------------------

    Vec3 position = {0.0f, 0.0f, 0.0f};
    Vec3 rotation = {0.0f, 0.0f, 0.0f}; // degrees, applied X then Y then Z
    Vec3 scale    = {1.0f, 1.0f, 1.0f};

    // --------------------------------------------------------
    // APPEARANCE
    // --------------------------------------------------------

    float r = 1.0f, g = 1.0f, b = 1.0f;
    GLuint texture = 0;      // 0 = untextured, falls back to r/g/b
    bool useLighting = true; // cheap fixed-direction diffuse, see Mesh.cpp

    // Always draws in WORLD space against Absolut::ActiveProjection
    // (see CameraTransform.h) - unlike Quad, Mesh has no SCREEN anchor.
    void draw();

private:

    std::vector<MeshVertex> vertices;
    std::vector<unsigned short> indices; // empty => glDrawArrays, no EBO

    GLuint vbo = 0;
    GLuint ibo = 0;
    bool uploaded = false;

    void upload();
    void releaseGL();

    static Mesh FromVertexList(std::vector<MeshVertex> verts);

    static Mesh FromIndexedList(
        std::vector<MeshVertex> verts,
        std::vector<unsigned short> idx
    );
};

}

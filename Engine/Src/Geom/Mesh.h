#pragma once

#include "Engine/dependencies/include.h"
#include <string>
#include <vector>
#include <memory>

#include "Math/Vector.h"   // Vec3

namespace Absolut
{

// ============================================================
// MESH VERTEX
//
// Layout uploaded to the GPU for every Mesh, whether it came
// from a procedural generator (CreateCube/CreatePyramid/etc),
// was loaded from a glTF file via LoadFromGLTF(), or is a
// skinned mesh being re-uploaded every frame. Matches the
// aPosition/aNormal/aTexCoord attributes bound in Mesh.cpp.
//
// Skinning is done entirely on the CPU (see Mesh.cpp), so this
// struct intentionally does NOT carry joint indices/weights -
// those live in the separate, CPU-only SkinVertex struct below
// and never touch the GPU.
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

    // Owns GL buffer objects (and, if loaded from glTF, possibly a
    // GL texture) - move-only, no implicit copies.
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

    // Loads mesh[0]/primitive[0] geometry from a .gltf/.glb file.
    // If the primitive's material has a baseColorTexture, it is
    // decoded and uploaded as a GL texture (owned by this Mesh,
    // replacing whatever `texture` was previously set). If the
    // node that references the mesh has a skin, joint weights and
    // animation clips are loaded too - see GetAnimationCount() /
    // SetAnimation() / UpdateAnimation() below. Skins with more
    // than kMaxJoints joints are loaded as a static mesh instead
    // (logged, not a hard failure).
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

    // Backface culling (GL_BACK, CCW front face). All procedural
    // primitives below (CreateCube/CreatePyramid/CreateCylinder/
    // CreateSphere/CreatePlane) are wound CCW as seen from the
    // outward normal, so this is safe to leave on by default.
    // Turn off per-mesh if a loaded model (LoadFromGLTF) has
    // inconsistent winding and ends up disappearing/inside-out.
    bool enableCulling = true;

    // --------------------------------------------------------
    // SKINNING / ANIMATION
    //
    // Only meaningful if LoadFromGLTF() found a skin (check
    // GetAnimationCount() > 0). Skinning is computed on the CPU
    // every UpdateAnimation() call and re-uploaded via
    // glBufferSubData - deliberately, since GLES2 doesn't
    // reliably give you enough vertex uniforms (spec minimum is
    // 128 vec4 = ~32 mat4) or vertex texture fetch (spec allows
    // zero vertex texture units) to do this on the GPU portably.
    // Call UpdateAnimation() once per frame before draw().
    // --------------------------------------------------------
    static constexpr int kMaxJoints = 512;

    int GetAnimationCount() const;
    std::string GetAnimationName(int index) const;

    // Switches the active clip and immediately samples frame 0,
    // so the mesh doesn't render bind pose for a frame while
    // waiting on the next UpdateAnimation(). Returns false (no-op)
    // if this mesh has no skin or the index/name doesn't exist.
    bool SetAnimation(int index, bool loop = true);
    bool SetAnimation(const std::string& name, bool loop = true);

    // Plays a sub-range of a clip, addressed by FRAME NUMBER rather
    // than seconds. frameStart/frameEnd are converted to seconds
    // internally as frame / fps - fps does NOT change playback
    // speed, it only tells this call how to interpret the frame
    // numbers. Match it to whatever your DCC tool (Blender/Maya)
    // used at export (commonly 24 or 30 fps - check your export
    // settings if the range looks off).
    //
    // Pass frameEnd = -1 to play to the end of the clip's full
    // duration.
    //
    // repeatTimes:
    //   <= 0  -> loop the range forever (same as SetAnimation(..., true))
    //    N>0  -> play the range exactly N times, then freeze on its
    //            last frame - check IsAnimationFinished()
    //
    // Returns false (no-op) if this mesh has no skin, the clip
    // name/index doesn't exist, or fps <= 0.
    bool PlayAnimation(
        int frameStart,
        int frameEnd,
        const std::string& animName,
        int repeatTimes = 0,
        float fps = 30.0f
    );

    bool PlayAnimation(
        int frameStart,
        int frameEnd,
        int index,
        int repeatTimes = 0,
        float fps = 30.0f
    );

    // True once a PlayAnimation()/SetAnimation(..., false) call has
    // played out all its repeats and frozen on the last frame of
    // its range. Always false for looping playback.
    bool IsAnimationFinished() const;

    // Freezes on whatever pose is currently displayed.
    void StopAnimation();

    // Advances the active clip and re-skins the CPU vertex copy.
    // No-op if there's no skin or no active clip.
    void UpdateAnimation(float deltaSeconds);

    // Always draws in WORLD space against Absolut::ActiveProjection
    // (see CameraTransform.h) - unlike Quad, Mesh has no SCREEN anchor.
    void draw();

private:
    std::vector<MeshVertex> vertices;
    std::vector<unsigned short> indices; // empty => glDrawArrays, no EBO

    GLuint vbo = 0;
    GLuint ibo = 0;
    bool uploaded = false;

    // True once LoadFromGLTF() created `texture` itself (rather than
    // the caller assigning an externally-owned GL texture) - only
    // then does this Mesh delete it in releaseGL()/on destruction.
    bool ownsTexture = false;

    // ----------------------------------------------------------
    // SKINNING (CPU)
    //
    // SkinVertex is intentionally separate from MeshVertex: it's
    // CPU-only bookkeeping (never uploaded to the GPU), so keeping
    // it out of MeshVertex means unskinned meshes/shaders/attribute
    // layouts are completely untouched by any of this.
    // ----------------------------------------------------------
    struct SkinVertex
    {
        unsigned short joints[4] = {0, 0, 0, 0};
        float weights[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    };

    // Full node hierarchy + joints + animation clips for this mesh's
    // skin. Defined entirely in Mesh.cpp (pimpl) so this header
    // doesn't need to know anything about glTF/animation internals.
    // Null == "this mesh has no skin, cannot be animated".
    //
    // Uses a custom deleter instead of std::unique_ptr<SkinData>'s
    // default one: std::default_delete<SkinData>::operator() is a
    // template, and instantiating it requires SkinData to be a
    // complete type AT THE INSTANTIATION SITE - which can be ANY
    // translation unit that only includes this header and happens
    // to implicitly instantiate Mesh's destructor/move-ops (e.g. a
    // std::vector<Mesh>, or some other class holding a Mesh member
    // and relying on its own implicit destructor). That's exactly
    // how you get "invalid application of 'sizeof' to incomplete
    // type SkinData" pointing into <bits/unique_ptr.h> instead of
    // into whatever file actually needed fixing.
    //
    // SkinDataDeleter::operator() below is an ordinary, non-template
    // member function - declared here, defined in Mesh.cpp where
    // SkinData is complete. Calling it from any other TU is just a
    // normal out-of-line function call, so no sizeof(SkinData) check
    // is ever instantiated outside Mesh.cpp, no matter where skin's
    // destructor ends up being invoked from.
    struct SkinData;
    struct SkinDataDeleter
    {
        void operator()(SkinData* p) const;
    };
    std::unique_ptr<SkinData, SkinDataDeleter> skin;

    std::vector<MeshVertex> bindPoseVertices; // unskinned copy; only populated when `skin` is set
    std::vector<SkinVertex> skinVerts;        // parallel to bindPoseVertices

    bool vertexBufferIsDynamic = false; // GL_DYNAMIC_DRAW (skinned) vs GL_STATIC_DRAW
    bool verticesDirty = false;         // set by ApplySkinningCPU(), consumed by draw()

    void ApplySkinningCPU(); // blends bindPoseVertices+skinVerts -> vertices using skin's joint matrices

    void upload();
    void updateDynamicVertexBuffer(); // glBufferSubData path, used every frame for skinned meshes
    void releaseGL();

    static Mesh FromVertexList(std::vector<MeshVertex> verts);
    static Mesh FromIndexedList(
        std::vector<MeshVertex> verts,
        std::vector<unsigned short> idx
    );
};

}

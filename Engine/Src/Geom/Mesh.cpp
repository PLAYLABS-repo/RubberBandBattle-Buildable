#include "Mesh.h"

#include <GLES2/gl2.h>
#include "Engine/GLES2Render/CameraTransform.h"

#include <cmath>
#include <cstdio>
#include <algorithm>
#include <functional>

// ----------------------------------------------------------------
// TINYGLTF
//
// tiny_gltf.h is header-only but needs exactly one translation
// unit that defines TINYGLTF_IMPLEMENTATION before including it -
// this is that TU. Do not add these defines anywhere else or
// you'll get duplicate-symbol link errors.
//
// Geometry (positions/normals/uvs/indices), skin data (joint
// indices/weights/inverse-bind matrices), animation clips, AND
// base-color textures are all read out of glTF files here.
// STB_IMAGE_WRITE is switched off (we never write images out),
// but STB_IMAGE (reading) is NOT switched off, since textures are
// decoded through it - stb_image.h needs to be sitting next to
// tiny_gltf.h on your include path. You still need tiny_gltf.h and
// its json.hpp (nlohmann/json) too.
// ----------------------------------------------------------------



namespace Absolut
{

static GLuint meshProgram = 0;

static GLint aPosition = -1;
static GLint aNormal   = -1;
static GLint aTexCoord = -1;

static GLint uProjection  = -1;
static GLint uModel       = -1;
static GLint uTexture     = -1;
static GLint uColor       = -1;
static GLint uUseTexture  = -1;
static GLint uUseLighting = -1;

static bool meshShaderInitialized = false;


// ============================================================
// SHADERS
//
// Unchanged by skinning: skinned meshes are blended on the CPU
// (see the SKINNING section below) and uploaded as ordinary
// MeshVertex data, so the vertex/fragment shaders and attribute
// layout don't need to know anything about bones at all.
//
// Normals are transformed by the full model matrix with w=0
// (translation dropped). That's exact for rotation + uniform
// scale, and only an approximation for non-uniform scale (it
// should really use the inverse-transpose in that case) - fine
// for the primitives below, worth revisiting if you start
// squashing loaded meshes non-uniformly. The same caveat applies
// to the CPU skinning blend matrix used for skinned normals.
// ============================================================

static const char* MeshVertexShader =
    "attribute vec3 aPosition;\n"
    "attribute vec3 aNormal;\n"
    "attribute vec2 aTexCoord;\n"

    "uniform mat4 uProjection;\n"
    "uniform mat4 uModel;\n"

    "varying vec3 vNormal;\n"
    "varying vec2 vTexCoord;\n"

    "void main()\n"
    "{\n"
    "    gl_Position = uProjection * uModel * vec4(aPosition, 1.0);\n"
    "    vNormal = (uModel * vec4(aNormal, 0.0)).xyz;\n"
    "    vTexCoord = aTexCoord;\n"
    "}\n";


static const char* MeshFragmentShader =
    "precision mediump float;\n"

    "uniform sampler2D uTexture;\n"
    "uniform vec4 uColor;\n"
    "uniform float uUseTexture;\n"
    "uniform float uUseLighting;\n"

    "varying vec3 vNormal;\n"
    "varying vec2 vTexCoord;\n"

    "void main()\n"
    "{\n"
    "    vec4 baseColor = (uUseTexture > 0.5)\n"
    "        ? texture2D(uTexture, vTexCoord)\n"
    "        : uColor;\n"

    "    if (uUseLighting > 0.5)\n"
    "    {\n"
    "        vec3 n = normalize(vNormal);\n"
    "        vec3 lightDir = normalize(vec3(0.4, 0.8, 0.6));\n"
    "        float ndotl = max(dot(n, lightDir), 0.0);\n"
    "        float lighting = 0.35 + 0.65 * ndotl;\n"
    "        baseColor.rgb *= lighting;\n"
    "    }\n"

    "    gl_FragColor = baseColor;\n"
    "}\n";


// ============================================================
// SHADER COMPILATION
// ============================================================

static GLuint CompileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);

    if (!shader)
        return 0;

    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), 0, log);
        printf("Mesh shader compilation failed:\n%s\n", log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}


static bool InitMeshShader()
{
    if (meshShaderInitialized)
        return meshProgram != 0;

    meshShaderInitialized = true;

    GLuint vertexShader   = CompileShader(GL_VERTEX_SHADER,   MeshVertexShader);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, MeshFragmentShader);

    if (!vertexShader || !fragmentShader)
    {
        if (vertexShader)   glDeleteShader(vertexShader);
        if (fragmentShader) glDeleteShader(fragmentShader);
        return false;
    }

    meshProgram = glCreateProgram();

    if (!meshProgram)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glAttachShader(meshProgram, vertexShader);
    glAttachShader(meshProgram, fragmentShader);

    glBindAttribLocation(meshProgram, 0, "aPosition");
    glBindAttribLocation(meshProgram, 1, "aNormal");
    glBindAttribLocation(meshProgram, 2, "aTexCoord");

    glLinkProgram(meshProgram);

    GLint success = 0;
    glGetProgramiv(meshProgram, GL_LINK_STATUS, &success);

    if (!success)
    {
        char log[1024];
        glGetProgramInfoLog(meshProgram, sizeof(log), 0, log);
        printf("Mesh shader linking failed:\n%s\n", log);

        glDeleteProgram(meshProgram);
        meshProgram = 0;

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    aPosition = glGetAttribLocation(meshProgram, "aPosition");
    aNormal   = glGetAttribLocation(meshProgram, "aNormal");
    aTexCoord = glGetAttribLocation(meshProgram, "aTexCoord");

    uProjection  = glGetUniformLocation(meshProgram, "uProjection");
    uModel       = glGetUniformLocation(meshProgram, "uModel");
    uTexture     = glGetUniformLocation(meshProgram, "uTexture");
    uColor       = glGetUniformLocation(meshProgram, "uColor");
    uUseTexture  = glGetUniformLocation(meshProgram, "uUseTexture");
    uUseLighting = glGetUniformLocation(meshProgram, "uUseLighting");

    return true;
}


// ============================================================
// MATRIX HELPERS
//
// Same column-major convention as CameraTransform.h - reuse
// CameraMath::Identity/Multiply from there instead of forking
// a third copy of them.
// ============================================================

static void Translate(float* m, float x, float y, float z)
{
    float t[16];
    CameraMath::Identity(t);
    t[12] = x; t[13] = y; t[14] = z;

    float result[16];
    CameraMath::Multiply(result, m, t);

    for (int i = 0; i < 16; ++i)
        m[i] = result[i];
}


static void Scale(float* m, float x, float y, float z)
{
    float s[16];
    CameraMath::Identity(s);
    s[0] = x; s[5] = y; s[10] = z;

    float result[16];
    CameraMath::Multiply(result, m, s);

    for (int i = 0; i < 16; ++i)
        m[i] = result[i];
}


static void RotateX(float* m, float degrees)
{
    float angle = degrees * 3.14159265f / 180.0f;
    float c = cosf(angle);
    float s = sinf(angle);

    float r[16];
    CameraMath::Identity(r);
    r[5] = c;  r[6]  = s;
    r[9] = -s; r[10] = c;

    float result[16];
    CameraMath::Multiply(result, m, r);

    for (int i = 0; i < 16; ++i)
        m[i] = result[i];
}


static void RotateY(float* m, float degrees)
{
    float angle = degrees * 3.14159265f / 180.0f;
    float c = cosf(angle);
    float s = sinf(angle);

    float r[16];
    CameraMath::Identity(r);
    r[0] = c;  r[2]  = -s;
    r[8] = s;  r[10] = c;

    float result[16];
    CameraMath::Multiply(result, m, r);

    for (int i = 0; i < 16; ++i)
        m[i] = result[i];
}


static void RotateZ(float* m, float degrees)
{
    float angle = degrees * 3.14159265f / 180.0f;
    float c = cosf(angle);
    float s = sinf(angle);

    float r[16];
    CameraMath::Identity(r);
    r[0] = c;  r[1] = s;
    r[4] = -s; r[5] = c;

    float result[16];
    CameraMath::Multiply(result, m, r);

    for (int i = 0; i < 16; ++i)
        m[i] = result[i];
}


// ------------------------------------------------------------
// Quaternion, used only by the skinning/animation code below.
// Same column-major convention as the Rotate*() helpers above -
// verified against RotateX/RotateY's layout.
// ------------------------------------------------------------
struct Quat { float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f; };


static void QuatToMat4(const Quat& q, float* out)
{
    float x = q.x, y = q.y, z = q.z, w = q.w;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;

    out[0]  = 1 - 2 * (yy + zz); out[1]  = 2 * (xy + wz);     out[2]  = 2 * (xz - wy);     out[3]  = 0;
    out[4]  = 2 * (xy - wz);     out[5]  = 1 - 2 * (xx + zz); out[6]  = 2 * (yz + wx);     out[7]  = 0;
    out[8]  = 2 * (xz + wy);     out[9]  = 2 * (yz - wx);     out[10] = 1 - 2 * (xx + yy); out[11] = 0;
    out[12] = 0;                 out[13] = 0;                 out[14] = 0;                 out[15] = 1;
}


static void RotateQuat(float* m, const Quat& q)
{
    float rot[16];
    QuatToMat4(q, rot);

    float result[16];
    CameraMath::Multiply(result, m, rot);

    for (int i = 0; i < 16; ++i)
        m[i] = result[i];
}


static Quat QuatNormalize(const Quat& q)
{
    float len = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (len < 1e-8f)
        return Quat{0.0f, 0.0f, 0.0f, 1.0f};
    return Quat{q.x / len, q.y / len, q.z / len, q.w / len};
}


static Quat QuatSlerp(Quat a, Quat b, float t)
{
    float dot = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

    // Take the short path around the hypersphere.
    if (dot < 0.0f)
    {
        b = Quat{-b.x, -b.y, -b.z, -b.w};
        dot = -dot;
    }

    if (dot > 0.9995f)
    {
        // Nearly identical rotations - lerp + normalize avoids a
        // divide-by-near-zero in the sin() path below.
        Quat result{
            a.x + t * (b.x - a.x),
            a.y + t * (b.y - a.y),
            a.z + t * (b.z - a.z),
            a.w + t * (b.w - a.w)
        };
        return QuatNormalize(result);
    }

    float theta0 = acosf(dot);
    float theta  = theta0 * t;

    float sinTheta0 = sinf(theta0);
    float sinTheta  = sinf(theta);

    float s0 = cosf(theta) - dot * sinTheta / sinTheta0;
    float s1 = sinTheta / sinTheta0;

    return Quat{
        s0 * a.x + s1 * b.x,
        s0 * a.y + s1 * b.y,
        s0 * a.z + s1 * b.z,
        s0 * a.w + s1 * b.w
    };
}


// Builds a local TRS matrix the same way draw() builds the model
// matrix: start at identity, then translate, then rotate, then
// scale (each step postmultiplies), i.e. localMatrix = T * R * S.
static void ComputeLocalMatrix(const Vec3& t, const Quat& q, const Vec3& s, float* out)
{
    CameraMath::Identity(out);
    Translate(out, t.x, t.y, t.z);
    RotateQuat(out, q);
    Scale(out, s.x, s.y, s.z);
}


static inline void TransformPoint(const float* m, float x, float y, float z, float& ox, float& oy, float& oz)
{
    ox = m[0] * x + m[4] * y + m[8]  * z + m[12];
    oy = m[1] * x + m[5] * y + m[9]  * z + m[13];
    oz = m[2] * x + m[6] * y + m[10] * z + m[14];
}


static inline void TransformDir(const float* m, float x, float y, float z, float& ox, float& oy, float& oz)
{
    ox = m[0] * x + m[4] * y + m[8]  * z;
    oy = m[1] * x + m[5] * y + m[9]  * z;
    oz = m[2] * x + m[6] * y + m[10] * z;
}


// ============================================================
// SKINNING / ANIMATION DATA
//
// Full definition of Mesh::SkinData (forward-declared in Mesh.h).
// Everything here is CPU-only bookkeeping built once in
// LoadFromGLTF() and consumed every frame by UpdateAnimation().
//
// This has to live here, ABOVE the LIFETIME section below, rather
// than down near the rest of the skinning/animation code: Mesh
// holds a std::unique_ptr<SkinData, SkinDataDeleter>, and the
// deleter's operator() (defined further below) needs SkinData to
// be a complete type at the point it's defined - which in turn
// needs to happen before Mesh::~Mesh(), Mesh(Mesh&&), and
// Mesh::operator=(Mesh&&) are compiled, all of which happen
// immediately below.
// ============================================================

struct Mesh::SkinData
{
    struct Node
    {
        int parent = -1;

        // As authored in the glTF file - Sample() resets to these
        // every call before applying the active clip's channels, so
        // nodes untouched by the current animation still get a
        // correct (bind) pose instead of leaking the previous clip's.
        Vec3 bindTranslation{0.0f, 0.0f, 0.0f};
        Quat bindRotation{0.0f, 0.0f, 0.0f, 1.0f};
        Vec3 bindScale{1.0f, 1.0f, 1.0f};

        // Working pose for the current frame.
        Vec3 translation{0.0f, 0.0f, 0.0f};
        Quat rotation{0.0f, 0.0f, 0.0f, 1.0f};
        Vec3 scaleV{1.0f, 1.0f, 1.0f};

        float global[16]; // filled in by RecomputeGlobalTransforms()
    };

    struct Joint
    {
        int nodeIndex = -1;
        float inverseBind[16];
    };

    enum class Path { Translation, Rotation, Scale };
    enum class Interp { Linear, Step };

    struct Channel
    {
        int nodeIndex = -1;
        Path path = Path::Translation;
        Interp interp = Interp::Linear;
        std::vector<float> times;  // 1 per key
        std::vector<float> values; // 3 per key (T/S) or 4 per key (rotation)
    };

    struct Clip
    {
        std::string name;
        float duration = 0.0f;
        std::vector<Channel> channels;
    };

    std::vector<Node> nodes;   // ALL glTF nodes, not just joints - needed to
                               // walk from a joint up to the skeleton root
    std::vector<Joint> joints; // in the same order as JOINTS_0 indices
    std::vector<Clip> clips;

    int activeClip = -1;
    float clipTime = 0.0f;
    bool looping = true;

    std::vector<float> jointMatrices; // joints.size() * 16 floats

    void RecomputeGlobalTransforms();
    void Sample(float t);
    void ComputeJointMatrices();
};


// ------------------------------------------------------------
// Custom unique_ptr deleter for SkinData (declared in Mesh.h).
// Defined here, now that SkinData is a complete type - this is
// the ONE place in the whole program where SkinData's size is
// ever needed for destruction purposes. Every other translation
// unit just calls this ordinary out-of-line function; none of
// them need SkinData to be complete.
// ------------------------------------------------------------
void Mesh::SkinDataDeleter::operator()(SkinData* p) const
{
    delete p;
}


// ============================================================
// LIFETIME
// ============================================================

Mesh::~Mesh()
{
    releaseGL();
}


Mesh::Mesh(Mesh&& other) noexcept
{
    vertices     = std::move(other.vertices);
    indices      = std::move(other.indices);
    vbo          = other.vbo;
    ibo          = other.ibo;
    uploaded     = other.uploaded;
    position     = other.position;
    rotation     = other.rotation;
    scale        = other.scale;
    r = other.r; g = other.g; b = other.b;
    texture      = other.texture;
    useLighting  = other.useLighting;
    enableCulling = other.enableCulling;
    ownsTexture  = other.ownsTexture;

    skin              = std::move(other.skin);
    bindPoseVertices  = std::move(other.bindPoseVertices);
    skinVerts         = std::move(other.skinVerts);
    vertexBufferIsDynamic = other.vertexBufferIsDynamic;
    verticesDirty     = other.verticesDirty;

    other.vbo = 0;
    other.ibo = 0;
    other.uploaded = false;
    other.ownsTexture = false;
    other.texture = 0;
}


Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this == &other)
        return *this;

    releaseGL();

    vertices     = std::move(other.vertices);
    indices      = std::move(other.indices);
    vbo          = other.vbo;
    ibo          = other.ibo;
    uploaded     = other.uploaded;
    position     = other.position;
    rotation     = other.rotation;
    scale        = other.scale;
    r = other.r; g = other.g; b = other.b;
    texture      = other.texture;
    useLighting  = other.useLighting;
    enableCulling = other.enableCulling;
    ownsTexture  = other.ownsTexture;

    skin              = std::move(other.skin);
    bindPoseVertices  = std::move(other.bindPoseVertices);
    skinVerts         = std::move(other.skinVerts);
    vertexBufferIsDynamic = other.vertexBufferIsDynamic;
    verticesDirty     = other.verticesDirty;

    other.vbo = 0;
    other.ibo = 0;
    other.uploaded = false;
    other.ownsTexture = false;
    other.texture = 0;

    return *this;
}


void Mesh::releaseGL()
{
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (ibo) { glDeleteBuffers(1, &ibo); ibo = 0; }

    if (ownsTexture && texture)
    {
        glDeleteTextures(1, &texture);
        texture = 0;
        ownsTexture = false;
    }

    uploaded = false;
}


// ============================================================
// UPLOAD
// ============================================================

void Mesh::upload()
{
    if (uploaded)
        releaseGL();

    if (vertices.empty())
        return;

    // Skinned meshes get re-skinned and re-uploaded every frame via
    // updateDynamicVertexBuffer() (glBufferSubData), so their VBO is
    // allocated GL_DYNAMIC_DRAW; everything else is GL_STATIC_DRAW
    // exactly as before.
    vertexBufferIsDynamic = (skin != nullptr);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(MeshVertex),
        vertices.data(),
        vertexBufferIsDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW
    );

    if (!indices.empty())
    {
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            indices.size() * sizeof(unsigned short),
            indices.data(),
            GL_STATIC_DRAW
        );
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    uploaded = true;
    verticesDirty = false;
}


void Mesh::updateDynamicVertexBuffer()
{
    if (!uploaded || !vbo)
        return;

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(
        GL_ARRAY_BUFFER, 0,
        vertices.size() * sizeof(MeshVertex),
        vertices.data()
    );
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    verticesDirty = false;
}


// ============================================================
// DRAW
// ============================================================

void Mesh::draw()
{
    if (!InitMeshShader())
        return;

    if (!uploaded)
        upload();
    else if (verticesDirty && vertexBufferIsDynamic)
        updateDynamicVertexBuffer();

    if (!uploaded || vertices.empty())
        return;

    bool hasTexture = texture != 0;

    // ----------------------------------------------------------
    // MODEL: scale -> rotate (X, then Y, then Z) -> translate
    // ----------------------------------------------------------

    float model[16];
    CameraMath::Identity(model);

    Translate(model, position.x, position.y, position.z);
    RotateZ(model, rotation.z);
    RotateY(model, rotation.y);
    RotateX(model, rotation.x);
    Scale(model, scale.x, scale.y, scale.z);

    // ----------------------------------------------------------
    // SHADER
    // ----------------------------------------------------------

    glUseProgram(meshProgram);

    glUniformMatrix4fv(uProjection, 1, GL_FALSE, Absolut::ActiveProjection);
    glUniformMatrix4fv(uModel, 1, GL_FALSE, model);

    glUniform4f(uColor, r, g, b, 1.0f);
    glUniform1f(uUseTexture, hasTexture ? 1.0f : 0.0f);
    glUniform1f(uUseLighting, useLighting ? 1.0f : 0.0f);

    glEnable(GL_DEPTH_TEST);

    // ----------------------------------------------------------
    // CULLING
    //
    // All procedural primitives (AddQuad/AddTri and the CreateX
    // factories below) are wound CCW as seen from the outward
    // normal, so GL_BACK + GL_CCW is correct for them. Meshes
    // loaded via LoadFromGLTF() aren't guaranteed to follow that
    // convention - if a loaded model disappears or looks inside
    // out, set enableCulling = false on that instance rather than
    // assuming this code is wrong.
    // ----------------------------------------------------------

    if (enableCulling)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }

    if (hasTexture)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(uTexture, 0);
    }
    else
    {
        glDisable(GL_BLEND);
    }

    // ----------------------------------------------------------
    // ATTRIBUTES
    // ----------------------------------------------------------

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(aPosition);
    glEnableVertexAttribArray(aNormal);
    glEnableVertexAttribArray(aTexCoord);

    glVertexAttribPointer(
        aPosition, 3, GL_FLOAT, GL_FALSE,
        sizeof(MeshVertex), (const void*)offsetof(MeshVertex, px)
    );

    glVertexAttribPointer(
        aNormal, 3, GL_FLOAT, GL_FALSE,
        sizeof(MeshVertex), (const void*)offsetof(MeshVertex, nx)
    );

    glVertexAttribPointer(
        aTexCoord, 2, GL_FLOAT, GL_FALSE,
        sizeof(MeshVertex), (const void*)offsetof(MeshVertex, u)
    );

    // ----------------------------------------------------------
    // DRAW
    // ----------------------------------------------------------

    if (ibo)
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_SHORT, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    else
    {
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());
    }

    // ----------------------------------------------------------
    // CLEANUP
    // ----------------------------------------------------------

    glDisableVertexAttribArray(aPosition);
    glDisableVertexAttribArray(aNormal);
    glDisableVertexAttribArray(aTexCoord);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (hasTexture)
        glBindTexture(GL_TEXTURE_2D, 0);

    // Don't leak culling state into whatever draws next (Quad,
    // text, etc. may assume GL_CULL_FACE is off).
    glDisable(GL_CULL_FACE);

    glUseProgram(0);
}


// ============================================================
// FACTORY HELPERS
// ============================================================

Mesh Mesh::FromVertexList(std::vector<MeshVertex> verts)
{
    Mesh m;
    m.vertices = std::move(verts);
    return m;
}


Mesh Mesh::FromIndexedList(std::vector<MeshVertex> verts, std::vector<unsigned short> idx)
{
    Mesh m;
    m.vertices = std::move(verts);
    m.indices  = std::move(idx);
    return m;
}


// ------------------------------------------------------------
// Local (non-member) generation helpers - shared by the
// primitive factories below.
// ------------------------------------------------------------

namespace
{
    Vec3 Cross(const Vec3& a, const Vec3& b)
    {
        return Vec3{
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    Vec3 Sub(const Vec3& a, const Vec3& b)
    {
        return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
    }

    Vec3 Normalize(const Vec3& v)
    {
        float len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
        if (len < 1e-8f)
            return Vec3{0.0f, 1.0f, 0.0f};
        return Vec3{v.x / len, v.y / len, v.z / len};
    }

    MeshVertex MakeVertex(const Vec3& p, const Vec3& n, float u, float v)
    {
        MeshVertex mv;
        mv.px = p.x; mv.py = p.y; mv.pz = p.z;
        mv.nx = n.x; mv.ny = n.y; mv.nz = n.z;
        mv.u  = u;   mv.v  = v;
        return mv;
    }

    // Flat-shaded quad (4 unique verts + normal, shared UV rect).
    // Winding: v0 -> v1 -> v2 -> v3 should be CCW as seen from
    // the direction the normal points.
    void AddQuad(
        std::vector<MeshVertex>& verts,
        const Vec3& v0, const Vec3& v1,
        const Vec3& v2, const Vec3& v3,
        const Vec3& normal)
    {
        verts.push_back(MakeVertex(v0, normal, 0.0f, 0.0f));
        verts.push_back(MakeVertex(v1, normal, 1.0f, 0.0f));
        verts.push_back(MakeVertex(v2, normal, 1.0f, 1.0f));

        verts.push_back(MakeVertex(v0, normal, 0.0f, 0.0f));
        verts.push_back(MakeVertex(v2, normal, 1.0f, 1.0f));
        verts.push_back(MakeVertex(v3, normal, 0.0f, 1.0f));
    }

    // Flat-shaded triangle, normal computed from winding order.
    void AddTri(
        std::vector<MeshVertex>& verts,
        const Vec3& v0, const Vec3& v1, const Vec3& v2)
    {
        Vec3 normal = Normalize(Cross(Sub(v1, v0), Sub(v2, v0)));

        verts.push_back(MakeVertex(v0, normal, 0.0f, 0.0f));
        verts.push_back(MakeVertex(v1, normal, 1.0f, 0.0f));
        verts.push_back(MakeVertex(v2, normal, 0.5f, 1.0f));
    }
}


// ============================================================
// CUBE
// ============================================================

Mesh Mesh::CreateCube(float size)
{
    float h = size * 0.5f;
    std::vector<MeshVertex> verts;
    verts.reserve(36);

    Vec3 p000{-h,-h,-h}, p100{ h,-h,-h}, p110{ h, h,-h}, p010{-h, h,-h};
    Vec3 p001{-h,-h, h}, p101{ h,-h, h}, p111{ h, h, h}, p011{-h, h, h};

    AddQuad(verts, p001, p101, p111, p011, Vec3{0, 0, 1});  // front  (+Z)
    AddQuad(verts, p100, p000, p010, p110, Vec3{0, 0,-1});  // back   (-Z)
    AddQuad(verts, p101, p100, p110, p111, Vec3{1, 0, 0});  // right  (+X)
    AddQuad(verts, p000, p001, p011, p010, Vec3{-1,0, 0});  // left   (-X)
    AddQuad(verts, p011, p111, p110, p010, Vec3{0, 1, 0});  // top    (+Y)
    AddQuad(verts, p000, p100, p101, p001, Vec3{0,-1, 0});  // bottom (-Y)

    return FromVertexList(std::move(verts));
}


// ============================================================
// PYRAMID (square base)
// ============================================================

Mesh Mesh::CreatePyramid(float baseSize, float height)
{
    float h = baseSize * 0.5f;
    std::vector<MeshVertex> verts;
    verts.reserve(18);

    Vec3 a{-h, 0.0f,-h};
    Vec3 b{ h, 0.0f,-h};
    Vec3 c{ h, 0.0f, h};
    Vec3 d{-h, 0.0f, h};
    Vec3 apex{0.0f, height, 0.0f};

    AddQuad(verts, d, c, b, a, Vec3{0, -1, 0}); // base, facing down

    AddTri(verts, a, b, apex); // sides
    AddTri(verts, b, c, apex);
    AddTri(verts, c, d, apex);
    AddTri(verts, d, a, apex);

    return FromVertexList(std::move(verts));
}


// ============================================================
// PLANE
// ============================================================

Mesh Mesh::CreatePlane(float width, float depth)
{
    float hw = width * 0.5f;
    float hd = depth * 0.5f;

    std::vector<MeshVertex> verts;
    verts.reserve(6);

    AddQuad(
        verts,
        Vec3{-hw, 0.0f,  hd},
        Vec3{ hw, 0.0f,  hd},
        Vec3{ hw, 0.0f, -hd},
        Vec3{-hw, 0.0f, -hd},
        Vec3{0.0f, 1.0f, 0.0f}
    );

    return FromVertexList(std::move(verts));
}


// ============================================================
// CYLINDER
// ============================================================

Mesh Mesh::CreateCylinder(float radius, float height, int segments)
{
    segments = std::max(segments, 3);

    float halfH = height * 0.5f;
    const float TAU = 6.28318530718f;

    std::vector<MeshVertex> verts;
    std::vector<unsigned short> idx;

    // --- Side wall: smooth-shaded ring, indexed triangle strip. ---
    // Ring vertices duplicated once (seam) so UV.u can wrap 0..1
    // cleanly instead of jumping back at the last segment.
    unsigned short sideStart = 0;

    for (int i = 0; i <= segments; ++i)
    {
        float t = (float)i / (float)segments;
        float angle = t * TAU;
        float cx = cosf(angle);
        float cz = sinf(angle);

        Vec3 normal{cx, 0.0f, cz};

        verts.push_back(MakeVertex(
            Vec3{cx * radius, -halfH, cz * radius}, normal, t, 0.0f));

        verts.push_back(MakeVertex(
            Vec3{cx * radius,  halfH, cz * radius}, normal, t, 1.0f));
    }

    for (int i = 0; i < segments; ++i)
    {
        unsigned short bottom0 = sideStart + (unsigned short)(i * 2);
        unsigned short top0    = bottom0 + 1;
        unsigned short bottom1 = bottom0 + 2;
        unsigned short top1    = bottom0 + 3;

        idx.push_back(bottom0); idx.push_back(bottom1); idx.push_back(top1);
        idx.push_back(bottom0); idx.push_back(top1);    idx.push_back(top0);
    }

    // --- Caps: triangle fans, flat-shaded (own vertex set, not
    // shared with the side wall, so normals don't get averaged
    // across the hard edge). ---
    auto addCap = [&](float y, float normalY, bool reverseWinding)
    {
        unsigned short center = (unsigned short)verts.size();
        verts.push_back(MakeVertex(Vec3{0, y, 0}, Vec3{0, normalY, 0}, 0.5f, 0.5f));

        unsigned short ringStart = (unsigned short)verts.size();

        for (int i = 0; i <= segments; ++i)
        {
            float t = (float)i / (float)segments;
            float angle = t * TAU;
            float cx = cosf(angle);
            float cz = sinf(angle);

            verts.push_back(MakeVertex(
                Vec3{cx * radius, y, cz * radius},
                Vec3{0, normalY, 0},
                cx * 0.5f + 0.5f, cz * 0.5f + 0.5f
            ));
        }

        for (int i = 0; i < segments; ++i)
        {
            unsigned short v0 = ringStart + (unsigned short)i;
            unsigned short v1 = ringStart + (unsigned short)(i + 1);

            if (reverseWinding)
            {
                idx.push_back(center); idx.push_back(v1); idx.push_back(v0);
            }
            else
            {
                idx.push_back(center); idx.push_back(v0); idx.push_back(v1);
            }
        }
    };

    addCap(halfH,  1.0f, false); // top
    addCap(-halfH, -1.0f, true); // bottom, opposite winding so it faces down

    return FromIndexedList(std::move(verts), std::move(idx));
}


// ============================================================
// SPHERE (UV sphere)
// ============================================================

Mesh Mesh::CreateSphere(float radius, int latSegments, int lonSegments)
{
    latSegments = std::max(latSegments, 2);
    lonSegments = std::max(lonSegments, 3);

    const float PI  = 3.14159265f;
    const float TAU = 6.28318530718f;

    std::vector<MeshVertex> verts;
    std::vector<unsigned short> idx;

    for (int lat = 0; lat <= latSegments; ++lat)
    {
        float v = (float)lat / (float)latSegments;
        float phi = v * PI; // 0 (top) .. PI (bottom)

        float sinPhi = sinf(phi);
        float cosPhi = cosf(phi);

        for (int lon = 0; lon <= lonSegments; ++lon)
        {
            float u = (float)lon / (float)lonSegments;
            float theta = u * TAU;

            float sinTheta = sinf(theta);
            float cosTheta = cosf(theta);

            Vec3 normal{
                sinPhi * cosTheta,
                cosPhi,
                sinPhi * sinTheta
            };

            Vec3 pos{
                normal.x * radius,
                normal.y * radius,
                normal.z * radius
            };

            verts.push_back(MakeVertex(pos, normal, u, 1.0f - v));
        }
    }

    int stride = lonSegments + 1;

    for (int lat = 0; lat < latSegments; ++lat)
    {
        for (int lon = 0; lon < lonSegments; ++lon)
        {
            unsigned short i0 = (unsigned short)(lat * stride + lon);
            unsigned short i1 = (unsigned short)(i0 + stride);
            unsigned short i2 = (unsigned short)(i0 + 1);
            unsigned short i3 = (unsigned short)(i1 + 1);

            idx.push_back(i0); idx.push_back(i1); idx.push_back(i2);
            idx.push_back(i2); idx.push_back(i1); idx.push_back(i3);
        }
    }

    return FromIndexedList(std::move(verts), std::move(idx));
}


// ============================================================
// GLTF LOADING
// ============================================================

static bool EndsWith(const std::string& value, const std::string& suffix)
{
    if (suffix.size() > value.size())
        return false;

    return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}


// Reads a glTF accessor's data into out, one component set per
// element (e.g. 3 floats per vec3). Only handles the component
// types tinygltf's Meshes/Blender/Maya exporters actually emit
// for POSITION/NORMAL/TEXCOORD_0/animation-sampler/inverse-bind-
// matrix data (float) - extend here if you hit an exporter that
// does something unusual. Also used for animation sampler
// input/output (componentsPerElement = 1, 3, or 4) and inverse
// bind matrices (componentsPerElement = 16).
static bool ReadFloatAccessor(
    const tinygltf::Model& model,
    int accessorIndex,
    int componentsPerElement,
    std::vector<float>& out)
{
    if (accessorIndex < 0)
        return false;

    const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[view.buffer];

    if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
    {
        printf("Mesh::LoadFromGLTF: non-float accessor not supported\n");
        return false;
    }

    size_t stride = accessor.ByteStride(view);
    if (stride == 0)
        stride = componentsPerElement * sizeof(float);

    const unsigned char* base =
        buffer.data.data() + view.byteOffset + accessor.byteOffset;

    out.resize(accessor.count * componentsPerElement);

    for (size_t i = 0; i < accessor.count; ++i)
    {
        const float* src = reinterpret_cast<const float*>(base + i * stride);

        for (int c = 0; c < componentsPerElement; ++c)
            out[i * componentsPerElement + c] = src[c];
    }

    return true;
}


static bool ReadIndexAccessor(
    const tinygltf::Model& model,
    int accessorIndex,
    std::vector<unsigned short>& out)
{
    const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[view.buffer];

    const unsigned char* base =
        buffer.data.data() + view.byteOffset + accessor.byteOffset;

    out.resize(accessor.count);

    for (size_t i = 0; i < accessor.count; ++i)
    {
        unsigned int value = 0;

        switch (accessor.componentType)
        {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                value = base[i];
                break;

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                value = reinterpret_cast<const unsigned short*>(base)[i];
                break;

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                value = reinterpret_cast<const unsigned int*>(base)[i];
                break;

            default:
                printf("Mesh::LoadFromGLTF: unsupported index component type\n");
                return false;
        }

        if (value > 0xFFFF)
        {
            printf(
                "Mesh::LoadFromGLTF: index %u exceeds GLES2's unsigned short "
                "range - split this mesh into smaller pieces before export\n",
                value
            );
            return false;
        }

        out[i] = (unsigned short)value;
    }

    return true;
}


// JOINTS_0 is VEC4 of ubyte or ushort - always integer, never
// normalized, so this doesn't share code with ReadWeightsAccessor.
static bool ReadJointsAccessor(
    const tinygltf::Model& model,
    int accessorIndex,
    std::vector<unsigned short>& out) // 4 per vertex
{
    const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[view.buffer];

    const unsigned char* base =
        buffer.data.data() + view.byteOffset + accessor.byteOffset;
    size_t stride = accessor.ByteStride(view);

    out.resize(accessor.count * 4);

    for (size_t i = 0; i < accessor.count; ++i)
    {
        switch (accessor.componentType)
        {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            {
                size_t s = stride ? stride : 4 * sizeof(unsigned char);
                const unsigned char* src = base + i * s;
                for (int c = 0; c < 4; ++c) out[i * 4 + c] = src[c];
                break;
            }

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            {
                size_t s = stride ? stride : 4 * sizeof(unsigned short);
                const unsigned short* src =
                    reinterpret_cast<const unsigned short*>(base + i * s);
                for (int c = 0; c < 4; ++c) out[i * 4 + c] = src[c];
                break;
            }

            default:
                printf("Mesh::LoadFromGLTF: unsupported JOINTS_0 component type\n");
                return false;
        }
    }

    return true;
}


// WEIGHTS_0 is VEC4 of float, or normalized ubyte/ushort.
static bool ReadWeightsAccessor(
    const tinygltf::Model& model,
    int accessorIndex,
    std::vector<float>& out) // 4 per vertex
{
    const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
    const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
    const tinygltf::Buffer& buffer = model.buffers[view.buffer];

    const unsigned char* base =
        buffer.data.data() + view.byteOffset + accessor.byteOffset;
    size_t stride = accessor.ByteStride(view);

    out.resize(accessor.count * 4);

    for (size_t i = 0; i < accessor.count; ++i)
    {
        switch (accessor.componentType)
        {
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
            {
                size_t s = stride ? stride : 4 * sizeof(float);
                const float* src = reinterpret_cast<const float*>(base + i * s);
                for (int c = 0; c < 4; ++c) out[i * 4 + c] = src[c];
                break;
            }

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            {
                size_t s = stride ? stride : 4 * sizeof(unsigned char);
                const unsigned char* src = base + i * s;
                for (int c = 0; c < 4; ++c) out[i * 4 + c] = src[c] / 255.0f;
                break;
            }

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            {
                size_t s = stride ? stride : 4 * sizeof(unsigned short);
                const unsigned short* src =
                    reinterpret_cast<const unsigned short*>(base + i * s);
                for (int c = 0; c < 4; ++c) out[i * 4 + c] = src[c] / 65535.0f;
                break;
            }

            default:
                printf("Mesh::LoadFromGLTF: unsupported WEIGHTS_0 component type\n");
                return false;
        }
    }

    return true;
}


// Decodes a glTF image (already loaded into `image.image` as raw
// pixels by tinygltf/stb_image) into a new GL texture. Returns 0
// on failure - caller decides whether that's fatal.
static GLuint CreateGLTextureFromImage(const tinygltf::Image& image)
{
    if (image.image.empty() || image.width <= 0 || image.height <= 0)
    {
        printf(
            "Mesh::LoadFromGLTF: baseColorTexture has no decoded pixel data - "
            "is stb_image.h on the include path next to tiny_gltf.h?\n"
        );
        return 0;
    }

    GLenum format;
    switch (image.component)
    {
        case 1: format = GL_LUMINANCE;       break;
        case 2: format = GL_LUMINANCE_ALPHA; break;
        case 3: format = GL_RGB;             break;
        case 4: format = GL_RGBA;            break;
        default:
            printf(
                "Mesh::LoadFromGLTF: unsupported texture component count %d\n",
                image.component
            );
            return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    GLint prevAlign = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlign);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D, 0, format,
        image.width, image.height, 0,
        format, GL_UNSIGNED_BYTE, image.image.data()
    );

    glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlign);

    // No mipmaps: GLES2 can't mip NPOT textures without going
    // through a full power-of-two upload path, and glTF source
    // images aren't guaranteed POT. Plain bilinear + clamp is the
    // safe default; add POT-only mipmapping yourself if you need it.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    return tex;
}


// NOTE: struct Mesh::SkinData and Mesh::SkinDataDeleter::operator()
// are defined earlier in this file (right after the Quat/matrix
// helpers, before the LIFETIME section below) rather than here -
// they need to happen before Mesh::~Mesh()/move-ctor/move-assign
// are compiled, and those come first in the file.


void Mesh::SkinData::RecomputeGlobalTransforms()
{
    std::vector<bool> done(nodes.size(), false);

    std::function<void(int)> computeNode = [&](int i)
    {
        if (done[i])
            return;

        float local[16];
        ComputeLocalMatrix(nodes[i].translation, nodes[i].rotation, nodes[i].scaleV, local);

        if (nodes[i].parent < 0)
        {
            for (int k = 0; k < 16; ++k)
                nodes[i].global[k] = local[k];
        }
        else
        {
            computeNode(nodes[i].parent);
            CameraMath::Multiply(nodes[i].global, nodes[nodes[i].parent].global, local);
        }

        done[i] = true;
    };

    for (size_t i = 0; i < nodes.size(); ++i)
        computeNode((int)i);
}


void Mesh::SkinData::Sample(float t)
{
    // Reset every node to its bind pose first, so properties the
    // active clip doesn't drive (or clips with partial coverage)
    // don't inherit stale values from whatever played previously.
    for (Node& n : nodes)
    {
        n.translation = n.bindTranslation;
        n.rotation    = n.bindRotation;
        n.scaleV      = n.bindScale;
    }

    if (activeClip < 0 || activeClip >= (int)clips.size())
        return;

    const Clip& clip = clips[activeClip];

    for (const Channel& ch : clip.channels)
    {
        if (ch.nodeIndex < 0 || ch.nodeIndex >= (int)nodes.size() || ch.times.empty())
            continue;

        // Find the bracketing keyframes [i0, i1] for time t.
        size_t i1 = 0;
        while (i1 < ch.times.size() && ch.times[i1] < t)
            ++i1;

        size_t i0;
        float localT;

        if (i1 == 0)
        {
            i0 = i1 = 0;
            localT = 0.0f;
        }
        else if (i1 >= ch.times.size())
        {
            i0 = i1 = ch.times.size() - 1;
            localT = 0.0f;
        }
        else
        {
            i0 = i1 - 1;
            float span = ch.times[i1] - ch.times[i0];
            localT = (span > 1e-6f) ? (t - ch.times[i0]) / span : 0.0f;
        }

        if (ch.interp == Interp::Step)
            localT = 0.0f; // snap to i0, no blending

        Node& node = nodes[ch.nodeIndex];

        switch (ch.path)
        {
            case Path::Translation:
            {
                const float* a = &ch.values[i0 * 3];
                const float* b = &ch.values[i1 * 3];
                node.translation = Vec3{
                    a[0] + (b[0] - a[0]) * localT,
                    a[1] + (b[1] - a[1]) * localT,
                    a[2] + (b[2] - a[2]) * localT
                };
                break;
            }

            case Path::Scale:
            {
                const float* a = &ch.values[i0 * 3];
                const float* b = &ch.values[i1 * 3];
                node.scaleV = Vec3{
                    a[0] + (b[0] - a[0]) * localT,
                    a[1] + (b[1] - a[1]) * localT,
                    a[2] + (b[2] - a[2]) * localT
                };
                break;
            }

            case Path::Rotation:
            {
                const float* a = &ch.values[i0 * 4];
                const float* b = &ch.values[i1 * 4];
                Quat qa{a[0], a[1], a[2], a[3]};
                Quat qb{b[0], b[1], b[2], b[3]};
                node.rotation = (ch.interp == Interp::Step) ? qa : QuatSlerp(qa, qb, localT);
                break;
            }
        }
    }
}


void Mesh::SkinData::ComputeJointMatrices()
{
    RecomputeGlobalTransforms();

    jointMatrices.resize(joints.size() * 16);

    for (size_t j = 0; j < joints.size(); ++j)
    {
        const float* global = nodes[joints[j].nodeIndex].global;
        CameraMath::Multiply(&jointMatrices[j * 16], global, joints[j].inverseBind);
    }
}


// ============================================================
// SKINNING (CPU)
// ============================================================

void Mesh::ApplySkinningCPU()
{
    if (!skin || skin->jointMatrices.empty())
        return;

    size_t n = bindPoseVertices.size();
    vertices.resize(n);

    for (size_t i = 0; i < n; ++i)
    {
        const MeshVertex& src = bindPoseVertices[i];
        const SkinVertex& sv  = skinVerts[i];
        MeshVertex& dst = vertices[i];

        dst.u = src.u;
        dst.v = src.v;

        float blend[16] = {0};
        bool anyWeight = false;

        for (int k = 0; k < 4; ++k)
        {
            float w = sv.weights[k];
            if (w <= 0.0f)
                continue;

            unsigned short j = sv.joints[k];
            if (j >= skin->joints.size())
                continue; // malformed data - skip rather than read OOB

            anyWeight = true;

            const float* jm = &skin->jointMatrices[j * 16];
            for (int e = 0; e < 16; ++e)
                blend[e] += jm[e] * w;
        }

        // Vertices with no weight at all (shouldn't normally happen
        // on a skinned mesh, but malformed exports exist) fall back
        // to the identity blend, i.e. bind pose.
        if (!anyWeight)
            CameraMath::Identity(blend);

        TransformPoint(blend, src.px, src.py, src.pz, dst.px, dst.py, dst.pz);
        TransformDir(blend, src.nx, src.ny, src.nz, dst.nx, dst.ny, dst.nz);
    }

    verticesDirty = true;
}


int Mesh::GetAnimationCount() const
{
    return skin ? (int)skin->clips.size() : 0;
}


std::string Mesh::GetAnimationName(int index) const
{
    if (!skin || index < 0 || index >= (int)skin->clips.size())
        return {};

    return skin->clips[index].name;
}


bool Mesh::SetAnimation(int index, bool loop)
{
    if (!skin || index < 0 || index >= (int)skin->clips.size())
        return false;

    skin->activeClip = index;
    skin->clipTime = 0.0f;
    skin->looping = loop;

    // Snap to frame 0 immediately so the mesh doesn't render bind
    // pose for a frame while waiting on the caller's next
    // UpdateAnimation().
    skin->Sample(0.0f);
    skin->ComputeJointMatrices();
    ApplySkinningCPU();

    return true;
}


bool Mesh::SetAnimation(const std::string& name, bool loop)
{
    if (!skin)
        return false;

    for (size_t i = 0; i < skin->clips.size(); ++i)
        if (skin->clips[i].name == name)
            return SetAnimation((int)i, loop);

    return false;
}


void Mesh::StopAnimation()
{
    if (skin)
        skin->activeClip = -1;
}


void Mesh::UpdateAnimation(float deltaSeconds)
{
    if (!skin || skin->activeClip < 0)
        return;

    const SkinData::Clip& clip = skin->clips[skin->activeClip];

    skin->clipTime += deltaSeconds;

    if (skin->looping)
    {
        if (clip.duration > 0.0f)
            skin->clipTime = fmodf(skin->clipTime, clip.duration);
    }
    else if (skin->clipTime > clip.duration)
    {
        skin->clipTime = clip.duration;
    }

    skin->Sample(skin->clipTime);
    skin->ComputeJointMatrices();
    ApplySkinningCPU();
}


// ============================================================
// GLTF LOAD ENTRY POINT
// ============================================================

bool Mesh::LoadFromGLTF(const std::string& path)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool ok = EndsWith(path, ".glb")
        ? loader.LoadBinaryFromFile(&model, &err, &warn, path)
        : loader.LoadASCIIFromFile(&model, &err, &warn, path);

    if (!warn.empty())
        printf("Mesh::LoadFromGLTF warning (%s): %s\n", path.c_str(), warn.c_str());

    if (!ok)
    {
        printf("Mesh::LoadFromGLTF failed (%s): %s\n", path.c_str(), err.c_str());
        return false;
    }

    if (model.meshes.empty() || model.meshes[0].primitives.empty())
    {
        printf("Mesh::LoadFromGLTF: no mesh/primitive found in %s\n", path.c_str());
        return false;
    }

    // NOTE: only mesh[0]/primitive[0] is loaded, same limitation as
    // before this change - multi-primitive meshes (e.g. one mesh
    // split across several materials) will silently drop everything
    // past the first primitive.
    const tinygltf::Primitive& prim = model.meshes[0].primitives[0];

    if (prim.mode != TINYGLTF_MODE_TRIANGLES)
    {
        printf("Mesh::LoadFromGLTF: only TRIANGLES primitives are supported\n");
        return false;
    }

    auto posIt = prim.attributes.find("POSITION");
    if (posIt == prim.attributes.end())
    {
        printf("Mesh::LoadFromGLTF: primitive has no POSITION attribute\n");
        return false;
    }

    std::vector<float> positions, normals, uvs;

    if (!ReadFloatAccessor(model, posIt->second, 3, positions))
        return false;

    size_t vertexCount = positions.size() / 3;

    auto normIt = prim.attributes.find("NORMAL");
    bool hasNormals = normIt != prim.attributes.end() &&
        ReadFloatAccessor(model, normIt->second, 3, normals);

    auto uvIt = prim.attributes.find("TEXCOORD_0");
    bool hasUVs = uvIt != prim.attributes.end() &&
        ReadFloatAccessor(model, uvIt->second, 2, uvs);

    std::vector<MeshVertex> newVerts(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i)
    {
        MeshVertex& mv = newVerts[i];

        mv.px = positions[i * 3 + 0];
        mv.py = positions[i * 3 + 1];
        mv.pz = positions[i * 3 + 2];

        if (hasNormals)
        {
            mv.nx = normals[i * 3 + 0];
            mv.ny = normals[i * 3 + 1];
            mv.nz = normals[i * 3 + 2];
        }
        else
        {
            // No NORMAL attribute in the source file - default to
            // "up" rather than a zero vector, which would make
            // the lighting term degenerate (NaN-free, just flat).
            mv.nx = 0.0f; mv.ny = 1.0f; mv.nz = 0.0f;
        }

        if (hasUVs)
        {
            mv.u = uvs[i * 2 + 0];
            mv.v = uvs[i * 2 + 1];
        }
        else
        {
            mv.u = 0.0f; mv.v = 0.0f;
        }
    }

    std::vector<unsigned short> newIndices;

    if (prim.indices >= 0)
    {
        if (!ReadIndexAccessor(model, prim.indices, newIndices))
            return false;
    }
    else if (vertexCount > 0xFFFF)
    {
        printf(
            "Mesh::LoadFromGLTF: non-indexed primitive has more than 65535 "
            "vertices, which GLES2's glDrawArrays index space can't address\n"
        );
        return false;
    }

    // ------------------------------------------------------------
    // TEXTURE (base color only - PBR metallic/roughness/normal/
    // emissive maps aren't read; extend here if you need them)
    // ------------------------------------------------------------
    if (prim.material >= 0 && prim.material < (int)model.materials.size())
    {
        const tinygltf::Material& mat = model.materials[prim.material];
        int texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;

        if (texIndex >= 0 && texIndex < (int)model.textures.size())
        {
            int imageIndex = model.textures[texIndex].source;

            if (imageIndex >= 0 && imageIndex < (int)model.images.size())
            {
                GLuint tex = CreateGLTextureFromImage(model.images[imageIndex]);

                if (tex)
                {
                    if (ownsTexture && texture)
                        glDeleteTextures(1, &texture);

                    texture = tex;
                    ownsTexture = true;
                }
            }
        }
    }

    // ------------------------------------------------------------
    // SKIN + BONE WEIGHTS
    // ------------------------------------------------------------
    auto jointsIt  = prim.attributes.find("JOINTS_0");
    auto weightsIt = prim.attributes.find("WEIGHTS_0");

    std::vector<unsigned short> jointsRaw;
    std::vector<float> weightsRaw;

    bool hasSkinAttribs =
        jointsIt != prim.attributes.end() &&
        weightsIt != prim.attributes.end() &&
        ReadJointsAccessor(model, jointsIt->second, jointsRaw) &&
        ReadWeightsAccessor(model, weightsIt->second, weightsRaw);

    // Find the node referencing mesh[0] to get its skin index - glTF
    // attaches skins to nodes, not meshes/primitives directly.
    int skinIndex = -1;
    if (hasSkinAttribs)
    {
        for (const tinygltf::Node& node : model.nodes)
        {
            if (node.mesh == 0 && node.skin >= 0)
            {
                skinIndex = node.skin;
                break;
            }
        }
    }

    skin.reset();

    if (hasSkinAttribs && skinIndex >= 0 && skinIndex < (int)model.skins.size())
    {
        const tinygltf::Skin& gltfSkin = model.skins[skinIndex];

        if (gltfSkin.joints.size() > (size_t)kMaxJoints)
        {
            printf(
                "Mesh::LoadFromGLTF: skin has %zu joints, exceeding the %d "
                "joint limit - loading %s as a static (unskinned) mesh instead\n",
                gltfSkin.joints.size(), kMaxJoints, path.c_str()
            );
        }
        else
        {
            auto newSkin = std::unique_ptr<SkinData, SkinDataDeleter>(new SkinData());

            // --- Full node hierarchy, needed to walk from any joint
            // up to the skeleton root when computing global poses ---
            newSkin->nodes.resize(model.nodes.size());

            for (size_t i = 0; i < model.nodes.size(); ++i)
            {
                const tinygltf::Node& gn = model.nodes[i];
                SkinData::Node& n = newSkin->nodes[i];

                if (gn.translation.size() == 3)
                    n.bindTranslation = Vec3{
                        (float)gn.translation[0],
                        (float)gn.translation[1],
                        (float)gn.translation[2]};

                if (gn.rotation.size() == 4)
                    n.bindRotation = Quat{
                        (float)gn.rotation[0], (float)gn.rotation[1],
                        (float)gn.rotation[2], (float)gn.rotation[3]};

                if (gn.scale.size() == 3)
                    n.bindScale = Vec3{
                        (float)gn.scale[0],
                        (float)gn.scale[1],
                        (float)gn.scale[2]};

                // A node given as a raw 4x4 `matrix` instead of TRS
                // isn't decomposed here - rare for animated rigs
                // (Blender/Maya both export TRS), but if you hit one,
                // decompose gn.matrix into T/R/S before this loop.

                n.translation = n.bindTranslation;
                n.rotation    = n.bindRotation;
                n.scaleV      = n.bindScale;
            }

            for (size_t i = 0; i < model.nodes.size(); ++i)
                for (int child : model.nodes[i].children)
                    if (child >= 0 && child < (int)newSkin->nodes.size())
                        newSkin->nodes[child].parent = (int)i;

            // --- Joints + inverse bind matrices ---
            std::vector<float> ibmFlat;
            bool haveIBM =
                gltfSkin.inverseBindMatrices >= 0 &&
                ReadFloatAccessor(model, gltfSkin.inverseBindMatrices, 16, ibmFlat);

            newSkin->joints.resize(gltfSkin.joints.size());

            for (size_t j = 0; j < gltfSkin.joints.size(); ++j)
            {
                newSkin->joints[j].nodeIndex = gltfSkin.joints[j];

                if (haveIBM)
                {
                    for (int e = 0; e < 16; ++e)
                        newSkin->joints[j].inverseBind[e] = ibmFlat[j * 16 + e];
                }
                else
                {
                    CameraMath::Identity(newSkin->joints[j].inverseBind);
                }
            }

            // --- Animation clips ---
            newSkin->clips.reserve(model.animations.size());

            for (const tinygltf::Animation& ga : model.animations)
            {
                SkinData::Clip clip;
                clip.name = ga.name;

                for (const tinygltf::AnimationChannel& gc : ga.channels)
                {
                    if (gc.target_node < 0 || gc.target_node >= (int)newSkin->nodes.size())
                        continue;

                    if (gc.sampler < 0 || gc.sampler >= (int)ga.samplers.size())
                        continue;

                    const tinygltf::AnimationSampler& sampler = ga.samplers[gc.sampler];

                    SkinData::Channel channel;
                    channel.nodeIndex = gc.target_node;

                    if (gc.target_path == "translation")
                        channel.path = SkinData::Path::Translation;
                    else if (gc.target_path == "rotation")
                        channel.path = SkinData::Path::Rotation;
                    else if (gc.target_path == "scale")
                        channel.path = SkinData::Path::Scale;
                    else
                        continue; // "weights" (morph targets) not supported

                    if (sampler.interpolation == "STEP")
                        channel.interp = SkinData::Interp::Step;
                    else
                        // CUBICSPLINE falls back to linear between the
                        // in-tangent/value/out-tangent triplet's middle
                        // value - not exact, but avoids misreading the
                        // accessor as if it were plain keyframes.
                        channel.interp = SkinData::Interp::Linear;

                    ReadFloatAccessor(model, sampler.input, 1, channel.times);

                    int comps = (channel.path == SkinData::Path::Rotation) ? 4 : 3;
                    ReadFloatAccessor(model, sampler.output, comps, channel.values);

                    if (!channel.times.empty())
                        clip.duration = std::max(clip.duration, channel.times.back());

                    clip.channels.push_back(std::move(channel));
                }

                newSkin->clips.push_back(std::move(clip));
            }

            skin = std::move(newSkin);
        }
    }

    // Per-vertex joint indices/weights - kept regardless of whether
    // `skin` ended up set (e.g. malformed skin index), just unused
    // in that case.
    skinVerts.assign(vertexCount, SkinVertex{});

    if (hasSkinAttribs)
    {
        for (size_t i = 0; i < vertexCount; ++i)
        {
            SkinVertex& sv = skinVerts[i];

            for (int c = 0; c < 4; ++c)
            {
                sv.joints[c]  = jointsRaw[i * 4 + c];
                sv.weights[c] = weightsRaw[i * 4 + c];
            }
        }
    }

    vertices = std::move(newVerts);
    indices  = std::move(newIndices);

    if (skin)
    {
        bindPoseVertices = vertices; // unskinned copy, re-blended from every frame
        skin->activeClip = -1;
        skin->clipTime = 0.0f;
        skin->Sample(0.0f); // bind pose is already correct until SetAnimation() is called
    }
    else
    {
        bindPoseVertices.clear();
        skinVerts.clear();
    }

    uploaded = false; // force re-upload next draw()
    verticesDirty = false;

    return true;
}

}

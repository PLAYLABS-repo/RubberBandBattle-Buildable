#include "Mesh.h"
#include "Engine/GLES2Render/CameraTransform.h"
#include "ufbx.h"

#include <cmath>
#include <cstdio>
#include <algorithm>
#include <functional>
#include <utility>
#include <string>
#include <vector>
#include <cctype>

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

    // Sub-range playback (set by SetAnimation()/PlayAnimation()).
    // rangeEnd defaults to the clip's full duration; playsRemaining
    // < 0 means "loop forever", otherwise it counts down and
    // finished latches true once it hits zero.
    float rangeStart = 0.0f;
    float rangeEnd = 0.0f;
    int playsRemaining = -1;
    bool finished = false;

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
// UFBX MODEL / ANIMATION LOADING
// ============================================================
// FBX and OBJ are loaded through ufbx. Animation stacks are baked by ufbx
// into simple translation/quaternion/scale keys and converted into the
// existing Mesh::SkinData format. This means the existing PlayAnimation(),
// SetAnimation(), UpdateAnimation() and CPU skinning code remain unchanged.
// ============================================================

static std::string MeshExtension(const std::string& path)
{
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return std::string();

    std::string ext = path.substr(dot);
    for (size_t i = 0; i < ext.size(); ++i)
        ext[i] = (char)std::tolower((unsigned char)ext[i]);
    return ext;
}

static void UFBXMatrixToFloat(const ufbx_matrix& m, float* out)
{
    // ufbx_matrix stores columns as cols[0..3], matching our column-major
    // CameraMath matrices.
    out[0]  = (float)m.m00; out[1]  = (float)m.m10; out[2]  = (float)m.m20; out[3]  = 0.0f;
    out[4]  = (float)m.m01; out[5]  = (float)m.m11; out[6]  = (float)m.m21; out[7]  = 0.0f;
    out[8]  = (float)m.m02; out[9]  = (float)m.m12; out[10] = (float)m.m22; out[11] = 0.0f;
    out[12] = (float)m.m03; out[13] = (float)m.m13; out[14] = (float)m.m23; out[15] = 1.0f;
}

bool LoadUFBXModel(Mesh& mesh, const std::string& path)
{
    ufbx_load_opts opts = {};
    opts.load_external_files = true;
    opts.ignore_missing_external_files = true;
    opts.generate_missing_normals = true;
    opts.target_axes = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0f;

    ufbx_error error = {};
    ufbx_scene* scene = ufbx_load_file(path.c_str(), &opts, &error);

    if (!scene)
    {
        printf("Mesh::Load: ufbx failed to load '%s': %s\n",
               path.c_str(), error.description.data);
        return false;
    }

    if (scene->meshes.count == 0)
    {
        printf("Mesh::Load: no mesh found in '%s'\n", path.c_str());
        ufbx_free_scene(scene);
        return false;
    }

    // The Mesh class is one drawable mesh. Use the first source mesh,
    // matching the old LoadFromGLTF() one-mesh API.
    ufbx_mesh* src = scene->meshes.data[0];
    if (!src)
    {
        ufbx_free_scene(scene);
        return false;
    }

    std::vector<MeshVertex> newVertices;
    std::vector<Mesh::SkinVertex> newSkinVerts;

    size_t estimated = 0;
    for (size_t fi = 0; fi < src->faces.count; ++fi)
        estimated += src->faces.data[fi].num_indices >= 3
            ? (src->faces.data[fi].num_indices - 2) * 3 : 0;
    newVertices.reserve(estimated);

    // Find a skin deformer, if the model has one.
    ufbx_skin_deformer* srcSkin =
        src->skin_deformers.count ? src->skin_deformers.data[0] : NULL;

    if (srcSkin)
        newSkinVerts.reserve(estimated);

    std::vector<uint32_t> triIndices;
    size_t maxFaceTriangles = src->max_face_triangles;
    if (maxFaceTriangles < 1)
        maxFaceTriangles = 1;
    triIndices.resize(maxFaceTriangles * 3);

    for (size_t fi = 0; fi < src->faces.count; ++fi)
    {
        ufbx_face face = src->faces.data[fi];
        if (face.num_indices < 3)
            continue;

        uint32_t numTriangles = ufbx_triangulate_face(
            triIndices.data(),
            triIndices.size(),
            src,
            face
        );

        for (uint32_t ti = 0; ti < numTriangles * 3; ++ti)
        {
            uint32_t cornerIndex = triIndices[ti];
            uint32_t vertexIndex = src->vertex_indices.data[cornerIndex];

            ufbx_vec3 p = ufbx_get_vertex_vec3(
                &src->vertex_position, cornerIndex);

            ufbx_vec3 n = {0, 1, 0};
            if (src->vertex_normal.exists)
                n = ufbx_get_vertex_vec3(&src->vertex_normal, cornerIndex);

            ufbx_vec2 uv = {0, 0};
            if (src->vertex_uv.exists)
                uv = ufbx_get_vertex_vec2(&src->vertex_uv, cornerIndex);

            MeshVertex v;
            v.px = (float)p.x;
            v.py = (float)p.y;
            v.pz = (float)p.z;
            v.nx = (float)n.x;
            v.ny = (float)n.y;
            v.nz = (float)n.z;
            v.u = (float)uv.x;
            v.v = (float)uv.y;
            newVertices.push_back(v);

            if (srcSkin)
            {
                Mesh::SkinVertex sv;

                if (vertexIndex < srcSkin->vertices.count)
                {
                    ufbx_skin_vertex sw = srcSkin->vertices.data[vertexIndex];

                    float total = 0.0f;
                    int used = 0;

                    for (size_t wi = 0;
                         wi < sw.num_weights && used < 4;
                         ++wi)
                    {
                        size_t weightIndex =
                            (size_t)sw.weight_begin + wi;
                        if (weightIndex >= srcSkin->weights.count)
                            break;

                        ufbx_skin_weight w =
                            srcSkin->weights.data[weightIndex];

                        if (w.cluster_index >= srcSkin->clusters.count)
                            continue;

                        sv.joints[used] = (unsigned short)w.cluster_index;
                        sv.weights[used] = (float)w.weight;
                        total += (float)w.weight;
                        ++used;
                    }

                    if (total > 1e-8f)
                    {
                        for (int k = 0; k < 4; ++k)
                            sv.weights[k] /= total;
                    }
                }

                newSkinVerts.push_back(sv);
            }
        }
    }

    if (newVertices.empty())
    {
        printf("Mesh::Load: '%s' has no triangle geometry\n", path.c_str());
        ufbx_free_scene(scene);
        return false;
    }

    mesh.releaseGL();
    mesh.vertices = std::move(newVertices);
    mesh.indices.clear();
    mesh.skin.reset();
    mesh.bindPoseVertices.clear();
    mesh.skinVerts.clear();
    mesh.vertexBufferIsDynamic = false;
    mesh.verticesDirty = false;
    mesh.uploaded = false;

    if (srcSkin && srcSkin->clusters.count > 0)
    {
        std::unique_ptr<Mesh::SkinData, Mesh::SkinDataDeleter> newSkin(
            new Mesh::SkinData());

        // Build the complete FBX node hierarchy. ufbx typed_id maps directly
        // to scene->nodes[] as documented by ufbx_baked_node.
        newSkin->nodes.resize(scene->nodes.count);
        for (size_t i = 0; i < scene->nodes.count; ++i)
            newSkin->nodes[i] = UFBXNodeToSkinNode(scene->nodes.data[i]);

        // The order of these joints is exactly the order used by
        // ufbx_skin_weight.cluster_index, so Mesh::SkinVertex can use it.
        newSkin->joints.resize(srcSkin->clusters.count);

        for (size_t ci = 0; ci < srcSkin->clusters.count; ++ci)
        {
            ufbx_skin_cluster* cluster = srcSkin->clusters.data[ci];
            Mesh::SkinData::Joint& joint = newSkin->joints[ci];
            joint.nodeIndex = cluster && cluster->bone_node
                ? (int)cluster->bone_node->typed_id : -1;

            if (cluster)
                UFBXMatrixToFloat(cluster->geometry_to_bone, joint.inverseBind);
            else
                CameraMath::Identity(joint.inverseBind);
        }

        // Bake every FBX animation stack into the engine's existing
        // translation/rotation/scale clip representation.
        for (size_t stackIndex = 0; stackIndex < scene->anim_stacks.count; ++stackIndex)
        {
            ufbx_anim_stack* stack = scene->anim_stacks.data[stackIndex];
            if (!stack || !stack->anim)
                continue;

            ufbx_error bakeError = {};
            ufbx_baked_anim* bake = ufbx_bake_anim(
                scene, stack->anim, NULL, &bakeError);

            if (!bake)
            {
                printf("Mesh::Load: couldn't bake FBX animation '%s': %s\\n",
                       stack->name.data, bakeError.description.data);
                continue;
            }

            Mesh::SkinData::Clip clip;
            clip.name = stack->name.data ? stack->name.data : "Animation";
            clip.duration = (float)bake->playback_duration;

            for (size_t bi = 0; bi < bake->nodes.count; ++bi)
            {
                const ufbx_baked_node& bn = bake->nodes.data[bi];
                if (bn.typed_id >= newSkin->nodes.size())
                    continue;

                if (bn.translation_keys.count > 0)
                {
                    Mesh::SkinData::Channel ch;
                    ch.nodeIndex = (int)bn.typed_id;
                    ch.path = Mesh::SkinData::Path::Translation;
                    ch.interp = Mesh::SkinData::Interp::Linear;
                    for (size_t k = 0; k < bn.translation_keys.count; ++k)
                    {
                        const ufbx_baked_vec3& key = bn.translation_keys.data[k];
                        ch.times.push_back((float)(key.time - bake->playback_time_begin));
                        ch.values.push_back((float)key.value.x);
                        ch.values.push_back((float)key.value.y);
                        ch.values.push_back((float)key.value.z);
                    }
                    clip.channels.push_back(std::move(ch));
                }

                if (bn.rotation_keys.count > 0)
                {
                    Mesh::SkinData::Channel ch;
                    ch.nodeIndex = (int)bn.typed_id;
                    ch.path = Mesh::SkinData::Path::Rotation;
                    ch.interp = Mesh::SkinData::Interp::Linear;
                    for (size_t k = 0; k < bn.rotation_keys.count; ++k)
                    {
                        const ufbx_baked_quat& key = bn.rotation_keys.data[k];
                        ch.times.push_back((float)(key.time - bake->playback_time_begin));
                        ch.values.push_back((float)key.value.x);
                        ch.values.push_back((float)key.value.y);
                        ch.values.push_back((float)key.value.z);
                        ch.values.push_back((float)key.value.w);
                    }
                    clip.channels.push_back(std::move(ch));
                }

                if (bn.scale_keys.count > 0)
                {
                    Mesh::SkinData::Channel ch;
                    ch.nodeIndex = (int)bn.typed_id;
                    ch.path = Mesh::SkinData::Path::Scale;
                    ch.interp = Mesh::SkinData::Interp::Linear;
                    for (size_t k = 0; k < bn.scale_keys.count; ++k)
                    {
                        const ufbx_baked_vec3& key = bn.scale_keys.data[k];
                        ch.times.push_back((float)(key.time - bake->playback_time_begin));
                        ch.values.push_back((float)key.value.x);
                        ch.values.push_back((float)key.value.y);
                        ch.values.push_back((float)key.value.z);
                    }
                    clip.channels.push_back(std::move(ch));
                }
            }

            if (!clip.channels.empty())
                newSkin->clips.push_back(std::move(clip));

            ufbx_free_baked_anim(bake);
        }

        if (!newSkin->joints.empty())
        {
            mesh.skin = std::move(newSkin);
            mesh.bindPoseVertices = mesh.vertices;
            mesh.skinVerts = std::move(newSkinVerts);
            mesh.vertexBufferIsDynamic = true;

            mesh.skin->activeClip = -1;
            mesh.skin->clipTime = 0.0f;
            mesh.skin->rangeStart = 0.0f;
            mesh.skin->rangeEnd = 0.0f;
            mesh.skin->playsRemaining = -1;
            mesh.skin->finished = false;
        }
    }

    // The source scene is no longer needed after converting the mesh and
    // baking the animation stacks into Mesh::SkinData.
    ufbx_free_scene(scene);

    return true;
}

bool Mesh::Load(const std::string& path)
{
    std::string ext = MeshExtension(path);

    if (ext == ".fbx" || ext == ".obj")
        return LoadUFBXModel(*this, path);

    printf("Mesh::Load: unsupported format: %s\n", path.c_str());
    printf("Mesh::Load: supported formats: .fbx and .obj\n");
    return false;
}

bool Mesh::SourceTexture(
    const std::string& source,
    const std::string& texturePath)
{
    (void)source;

    FREE_IMAGE_FORMAT fmt = FreeImage_GetFileType(texturePath.c_str(), 0);
    if (fmt == FIF_UNKNOWN)
        fmt = FreeImage_GetFIFFromFilename(texturePath.c_str());

    if (fmt == FIF_UNKNOWN)
        return false;

    FIBITMAP* image = FreeImage_Load(fmt, texturePath.c_str(), 0);
    if (!image)
        return false;

    FIBITMAP* rgba = FreeImage_ConvertTo32Bits(image);
    FreeImage_Unload(image);
    if (!rgba)
        return false;

    int width = (int)FreeImage_GetWidth(rgba);
    int height = (int)FreeImage_GetHeight(rgba);
    BYTE* bits = FreeImage_GetBits(rgba);
    unsigned pitch = FreeImage_GetPitch(rgba);

    if (!bits || width <= 0 || height <= 0)
    {
        FreeImage_Unload(rgba);
        return false;
    }

    std::vector<unsigned char> pixels(
        (size_t)width * (size_t)height * 4);

    for (int y = 0; y < height; ++y)
    {
        BYTE* srcRow = bits + (size_t)y * pitch;
        unsigned char* dstRow = pixels.data() +
            (size_t)(height - 1 - y) * (size_t)width * 4;

        for (int x = 0; x < width; ++x)
        {
            BYTE* p = srcRow + x * 4;
            dstRow[x * 4 + 0] = p[FI_RGBA_RED];
            dstRow[x * 4 + 1] = p[FI_RGBA_GREEN];
            dstRow[x * 4 + 2] = p[FI_RGBA_BLUE];
            dstRow[x * 4 + 3] = p[FI_RGBA_ALPHA];
        }
    }

    FreeImage_Unload(rgba);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA,
        width, height, 0,
        GL_RGBA, GL_UNSIGNED_BYTE,
        pixels.data()
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (mesh.ownsTexture && mesh.texture)
        glDeleteTextures(1, &mesh.texture);

    mesh.texture = tex;
    mesh.ownsTexture = true;
    return true;
}

} // namespace Absolut

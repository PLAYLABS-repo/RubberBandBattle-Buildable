#include "Mesh.h"

#include <GLES2/gl2.h>
#include "Engine/GLES2Render/CameraTransform.h"

#include <cmath>
#include <cstdio>
#include <algorithm>

// ----------------------------------------------------------------
// TINYGLTF
//
// tiny_gltf.h is header-only but needs exactly one translation
// unit that defines TINYGLTF_IMPLEMENTATION before including it -
// this is that TU. Do not add these defines anywhere else or
// you'll get duplicate-symbol link errors.
//
// We only ever read geometry (positions/normals/uvs/indices) out
// of glTF files here, never embedded images, so STB image support
// is switched off - that means you do NOT need stb_image.h /
// stb_image_write.h in the project for this to compile. You DO
// still need tiny_gltf.h and its json.hpp (nlohmann/json) sitting
// somewhere on the include path.
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
// Normals are transformed by the full model matrix with w=0
// (translation dropped). That's exact for rotation + uniform
// scale, and only an approximation for non-uniform scale (it
// should really use the inverse-transpose in that case) - fine
// for the primitives below, worth revisiting if you start
// squashing loaded meshes non-uniformly.
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

    other.vbo = 0;
    other.ibo = 0;
    other.uploaded = false;
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

    other.vbo = 0;
    other.ibo = 0;
    other.uploaded = false;

    return *this;
}


void Mesh::releaseGL()
{
    if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    if (ibo) { glDeleteBuffers(1, &ibo); ibo = 0; }
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

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(MeshVertex),
        vertices.data(),
        GL_STATIC_DRAW
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
// for POSITION/NORMAL/TEXCOORD_0 (float) - extend here if you
// hit an exporter that does something unusual.
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

    vertices = std::move(newVerts);
    indices  = std::move(newIndices);
    uploaded = false; // force re-upload next draw()

    return true;
}

}

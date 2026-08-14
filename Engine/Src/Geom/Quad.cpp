#include "Quad.h"

#include <GLES2/gl2.h>
#include "Engine/GLES2Render/CameraTransform.h"

#include <cmath>
#include <cstdio>

namespace Absolut
{

static GLuint quadProgram = 0;

static GLint aPosition = -1;
static GLint aTexCoord = -1;

static GLint uProjection = -1;
static GLint uModel = -1;
static GLint uTexture = -1;
static GLint uColor = -1;
static GLint uUseTexture = -1;

static bool quadShaderInitialized = false;


// ============================================================
// SHADERS
// ============================================================

static const char* QuadVertexShader =
    "attribute vec2 aPosition;\n"
    "attribute vec2 aTexCoord;\n"

    "uniform mat4 uProjection;\n"
    "uniform mat4 uModel;\n"

    "varying vec2 vTexCoord;\n"

    "void main()\n"
    "{\n"
    "    gl_Position = uProjection * uModel * "
    "                  vec4(aPosition, 0.0, 1.0);\n"

    "    vTexCoord = aTexCoord;\n"
    "}\n";


static const char* QuadFragmentShader =
    "precision mediump float;\n"

    "uniform sampler2D uTexture;\n"
    "uniform vec4 uColor;\n"
    "uniform float uUseTexture;\n"

    "varying vec2 vTexCoord;\n"

    "void main()\n"
    "{\n"
    "    if (uUseTexture > 0.5)\n"
    "    {\n"
    "        // Texture is used directly.\n"
    "        // Do NOT multiply RGB by uColor.\n"
    "        gl_FragColor = texture2D(uTexture, vTexCoord);\n"
    "    }\n"
    "    else\n"
    "    {\n"
    "        gl_FragColor = uColor;\n"
    "    }\n"
    "}\n";


// ============================================================
// SHADER COMPILATION
// ============================================================

static GLuint CompileShader(
    GLenum type,
    const char* source)
{
    GLuint shader =
        glCreateShader(type);

    if (!shader)
        return 0;

    glShaderSource(
        shader,
        1,
        &source,
        0
    );

    glCompileShader(shader);

    GLint success = 0;

    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &success
    );

    if (!success)
    {
        char log[1024];

        glGetShaderInfoLog(
            shader,
            sizeof(log),
            0,
            log
        );

        printf(
            "Quad shader compilation failed:\n%s\n",
            log
        );

        glDeleteShader(shader);

        return 0;
    }

    return shader;
}


// ============================================================
// INITIALIZE SHADER
// ============================================================

static bool InitQuadShader()
{
    if (quadShaderInitialized)
        return quadProgram != 0;

    quadShaderInitialized = true;

    GLuint vertexShader =
        CompileShader(
            GL_VERTEX_SHADER,
            QuadVertexShader
        );

    GLuint fragmentShader =
        CompileShader(
            GL_FRAGMENT_SHADER,
            QuadFragmentShader
        );

    if (!vertexShader ||
        !fragmentShader)
    {
        if (vertexShader)
            glDeleteShader(vertexShader);

        if (fragmentShader)
            glDeleteShader(fragmentShader);

        return false;
    }


    quadProgram =
        glCreateProgram();

    if (!quadProgram)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return false;
    }


    glAttachShader(
        quadProgram,
        vertexShader
    );

    glAttachShader(
        quadProgram,
        fragmentShader
    );


    glBindAttribLocation(
        quadProgram,
        0,
        "aPosition"
    );

    glBindAttribLocation(
        quadProgram,
        1,
        "aTexCoord"
    );


    glLinkProgram(
        quadProgram
    );


    GLint success = 0;

    glGetProgramiv(
        quadProgram,
        GL_LINK_STATUS,
        &success
    );

    if (!success)
    {
        char log[1024];

        glGetProgramInfoLog(
            quadProgram,
            sizeof(log),
            0,
            log
        );

        printf(
            "Quad shader linking failed:\n%s\n",
            log
        );

        glDeleteProgram(
            quadProgram
        );

        quadProgram = 0;

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return false;
    }


    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


    // --------------------------------------------------------
    // LOCATIONS
    // --------------------------------------------------------

    aPosition =
        glGetAttribLocation(
            quadProgram,
            "aPosition"
        );

    aTexCoord =
        glGetAttribLocation(
            quadProgram,
            "aTexCoord"
        );

    uProjection =
        glGetUniformLocation(
            quadProgram,
            "uProjection"
        );

    uModel =
        glGetUniformLocation(
            quadProgram,
            "uModel"
        );

    uTexture =
        glGetUniformLocation(
            quadProgram,
            "uTexture"
        );

    uColor =
        glGetUniformLocation(
            quadProgram,
            "uColor"
        );

    uUseTexture =
        glGetUniformLocation(
            quadProgram,
            "uUseTexture"
        );


    return true;
}


// ============================================================
// MATRIX HELPERS
// ============================================================

static void Identity(float* m)
{
    for (int i = 0; i < 16; ++i)
        m[i] = 0.0f;

    m[0]  = 1.0f;
    m[5]  = 1.0f;
    m[10] = 1.0f;
    m[15] = 1.0f;
}


static void Multiply(
    float* out,
    const float* a,
    const float* b)
{
    float result[16];

    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            result[col * 4 + row] =
                a[0 * 4 + row] *
                b[col * 4 + 0] +

                a[1 * 4 + row] *
                b[col * 4 + 1] +

                a[2 * 4 + row] *
                b[col * 4 + 2] +

                a[3 * 4 + row] *
                b[col * 4 + 3];
        }
    }

    for (int i = 0; i < 16; ++i)
        out[i] = result[i];
}


static void Translate(
    float* m,
    float x,
    float y)
{
    float t[16];

    Identity(t);

    t[12] = x;
    t[13] = y;

    float result[16];

    Multiply(
        result,
        m,
        t
    );

    for (int i = 0; i < 16; ++i)
        m[i] = result[i];
}


static void Rotate(
    float* m,
    float degrees)
{
    float angle =
        degrees *
        3.14159265f /
        180.0f;

    float c = cosf(angle);
    float s = sinf(angle);

    float r[16];

    Identity(r);

    r[0] = c;
    r[1] = s;

    r[4] = -s;
    r[5] = c;

    float result[16];

    Multiply(
        result,
        m,
        r
    );

    for (int i = 0; i < 16; ++i)
        m[i] = result[i];
}


// ============================================================
// ANCHOR
// ============================================================

void Quad::AnchorTo(
    QuadSpace from,
    QuadSpace to)
{
    anchorFrom = from;
    anchorTo   = to;
}


QuadSpace Quad::GetAnchorFrom() const
{
    return anchorFrom;
}


QuadSpace Quad::GetAnchorTo() const
{
    return anchorTo;
}


// ============================================================
// DRAW
// ============================================================

void Quad::draw()
{
    if (!InitQuadShader())
        return;


    bool hasTexture =
        texture != 0;


    // --------------------------------------------------------
    // MODEL
    // --------------------------------------------------------

    float model[16];

    Identity(model);


    Translate(
        model,
        x,
        y
    );


    // --------------------------------------------------------
    // SKEW
    // --------------------------------------------------------

    float sx =
        tanf(
            SkewX *
            3.14159265f /
            180.0f
        );

    float sy =
        tanf(
            SkewY *
            3.14159265f /
            180.0f
        );


    float skew[16];

    Identity(skew);

    skew[4] = -sy;
    skew[1] = -sx;


    float temp[16];

    Multiply(
        temp,
        model,
        skew
    );

    for (int i = 0; i < 16; ++i)
        model[i] = temp[i];


    // --------------------------------------------------------
    // PIVOT
    // --------------------------------------------------------

    Translate(
        model,
        PivotX,
        PivotY
    );

    Rotate(
        model,
        Rotation
    );

    Translate(
        model,
        -PivotX,
        -PivotY
    );


    // --------------------------------------------------------
    // BITMAP OFFSET
    // --------------------------------------------------------

    Translate(
        model,
        BitmapOffsetX,
        BitmapOffsetY
    );


    // --------------------------------------------------------
    // PROJECTION
    //
    // WORLD-anchored quads use the camera's current
    // projection (published by Camera::apply() into
    // Absolut::ActiveProjection - includes pan/zoom/rotation).
    //
    // SCREEN-anchored quads (HUD/UI) always use a fixed
    // screen-space ortho and ignore the camera entirely.
    // --------------------------------------------------------

    float screenProjection[16];

    float left   = 0.0f;
    float right  = 1280.0f;
    float top    = 0.0f;
    float bottom = 720.0f;

    Identity(screenProjection);

    screenProjection[0] =
        2.0f /
        (right - left);

    screenProjection[5] =
        2.0f /
        (top - bottom);

    screenProjection[10] =
        -1.0f;

    screenProjection[12] =
        -(right + left) /
        (right - left);

    screenProjection[13] =
        -(top + bottom) /
        (top - bottom);


    const float* projection =
        (GetAnchorFrom() == WORLD)
        ? Absolut::ActiveProjection
        : screenProjection;


    // --------------------------------------------------------
    // VERTICES
    // --------------------------------------------------------

    GLfloat vertices[] =
    {
        0.0f, 0.0f,
        u0, v0,

        w, 0.0f,
        u1, v0,

        w, h,
        u1, v1,


        0.0f, 0.0f,
        u0, v0,

        w, h,
        u1, v1,

        0.0f, h,
        u0, v1
    };


    // --------------------------------------------------------
    // SHADER
    // --------------------------------------------------------

    glUseProgram(
        quadProgram
    );


    glUniformMatrix4fv(
        uProjection,
        1,
        GL_FALSE,
        projection
    );

    glUniformMatrix4fv(
        uModel,
        1,
        GL_FALSE,
        model
    );


    // --------------------------------------------------------
    // COLOR
    // --------------------------------------------------------

    glUniform4f(
        uColor,
        r,
        g,
        b,
        1.0f
    );


    // --------------------------------------------------------
    // TEXTURE MODE
    // --------------------------------------------------------

    glUniform1f(
        uUseTexture,
        hasTexture ? 1.0f : 0.0f
    );


    // --------------------------------------------------------
    // BLENDING
    // --------------------------------------------------------

    if (hasTexture)
    {
        glEnable(GL_BLEND);

        glBlendFunc(
            GL_SRC_ALPHA,
            GL_ONE_MINUS_SRC_ALPHA
        );
    }
    else
    {
        glDisable(GL_BLEND);
    }


    // --------------------------------------------------------
    // TEXTURE
    // --------------------------------------------------------

    if (hasTexture)
    {
        glActiveTexture(
            GL_TEXTURE0
        );

        glBindTexture(
            GL_TEXTURE_2D,
            texture
        );

        glUniform1i(
            uTexture,
            0
        );
    }


    // --------------------------------------------------------
    // ATTRIBUTES
    // --------------------------------------------------------

    glEnableVertexAttribArray(
        aPosition
    );

    glEnableVertexAttribArray(
        aTexCoord
    );


    glVertexAttribPointer(
        aPosition,
        2,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(GLfloat),
        vertices
    );

    glVertexAttribPointer(
        aTexCoord,
        2,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(GLfloat),
        vertices + 2
    );


    // --------------------------------------------------------
    // DRAW
    // --------------------------------------------------------

    glDrawArrays(
        GL_TRIANGLES,
        0,
        6
    );


    // --------------------------------------------------------
    // CLEANUP
    // --------------------------------------------------------

    glDisableVertexAttribArray(
        aPosition
    );

    glDisableVertexAttribArray(
        aTexCoord
    );


    if (hasTexture)
    {
        glBindTexture(
            GL_TEXTURE_2D,
            0
        );
    }


    glUseProgram(0);
}

}

#define STB_TRUETYPE_IMPLEMENTATION
#include "Font.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

namespace Absolut
{

// Size of the baked glyph atlas. Bump this up if you use a large
// pixel size and characters start getting clipped/overlapping.
static const int ATLAS_WIDTH  = 512;
static const int ATLAS_HEIGHT = 512;


Text::Text()
{
    Texture = 0;
    AtlasWidth = 0;
    AtlasHeight = 0;

    Initialized = false;

    ColorR = 1.0f;
    ColorG = 1.0f;
    ColorB = 1.0f;
    ColorA = 1.0f;

    Program = 0;
    AttribPosition = -1;
    AttribTexCoord = -1;
    UniformProjection = -1;
    UniformColor = -1;
    UniformTexture = -1;

    VBO = 0;

    memset(Projection, 0, sizeof(Projection));
}


Text::~Text()
{
    Unload();
}


bool Text::LoadFont(
    const char* path,
    int size
)
{
    // ------------------------------------------------------------
    // Read the font file into memory
    // ------------------------------------------------------------

    FILE* file = fopen(path, "rb");

    if (!file)
    {
        printf(
            "Text: failed to open font file: %s\n",
            path
        );

        return false;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (fileSize <= 0)
    {
        printf(
            "Text: font file is empty: %s\n",
            path
        );

        fclose(file);

        return false;
    }

    unsigned char* ttfBuffer =
        (unsigned char*)malloc(fileSize);

    size_t bytesRead =
        fread(ttfBuffer, 1, fileSize, file);

    fclose(file);

    if (bytesRead != (size_t)fileSize)
    {
        printf(
            "Text: failed to read font file: %s\n",
            path
        );

        free(ttfBuffer);

        return false;
    }


    // ------------------------------------------------------------
    // Bake an ASCII glyph atlas with stb_truetype
    // ------------------------------------------------------------

    unsigned char* atlasBitmap =
        (unsigned char*)malloc(ATLAS_WIDTH * ATLAS_HEIGHT);

    int result = stbtt_BakeFontBitmap(
        ttfBuffer,
        0,
        (float)size,
        atlasBitmap,
        ATLAS_WIDTH,
        ATLAS_HEIGHT,
        FirstChar,
        NumChars,
        BakedChars
    );

    free(ttfBuffer);

    if (result <= 0)
    {
        printf(
            "Text: font atlas too small for font: %s (size %d)\n",
            path,
            size
        );

        free(atlasBitmap);

        return false;
    }

    AtlasWidth = ATLAS_WIDTH;
    AtlasHeight = ATLAS_HEIGHT;


    // ------------------------------------------------------------
    // Upload the atlas as a single-channel (alpha) GLES2 texture
    // ------------------------------------------------------------

    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        1
    );

    glGenTextures(
        1,
        &Texture
    );

    glBindTexture(
        GL_TEXTURE_2D,
        Texture
    );

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_ALPHA,
        AtlasWidth,
        AtlasHeight,
        0,
        GL_ALPHA,
        GL_UNSIGNED_BYTE,
        atlasBitmap
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glBindTexture(
        GL_TEXTURE_2D,
        0
    );

    free(atlasBitmap);


    // ------------------------------------------------------------
    // Build the GLES2 shader + VBO used to draw glyph quads
    // ------------------------------------------------------------

    if (!CreateShader())
    {
        printf(
            "Text: failed to create shader\n"
        );

        glDeleteTextures(1, &Texture);
        Texture = 0;

        return false;
    }


    // Sensible default projection (caller should override with the
    // real viewport size via SetProjection()).
    SetProjection(1280.0f, 720.0f);


    Initialized = true;

    return true;
}


GLuint Text::CompileShader(
    GLenum type,
    const char* source
)
{
    GLuint shader = glCreateShader(type);

    glShaderSource(
        shader,
        1,
        &source,
        0
    );

    glCompileShader(shader);


    GLint compiled = GL_FALSE;

    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &compiled
    );

    if (!compiled)
    {
        char log[512];

        glGetShaderInfoLog(
            shader,
            sizeof(log),
            0,
            log
        );

        printf(
            "Text: shader compile error: %s\n",
            log
        );

        glDeleteShader(shader);

        return 0;
    }

    return shader;
}


bool Text::CreateShader()
{
    static const char* vertexSource =
        "attribute vec2 aPosition;\n"
        "attribute vec2 aTexCoord;\n"
        "varying vec2 vTexCoord;\n"
        "uniform mat4 uProjection;\n"
        "void main()\n"
        "{\n"
        "    vTexCoord = aTexCoord;\n"
        "    gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);\n"
        "}\n";

    static const char* fragmentSource =
        "precision mediump float;\n"
        "varying vec2 vTexCoord;\n"
        "uniform sampler2D uTexture;\n"
        "uniform vec4 uColor;\n"
        "void main()\n"
        "{\n"
        "    float a = texture2D(uTexture, vTexCoord).a;\n"
        "    gl_FragColor = vec4(uColor.rgb, uColor.a * a);\n"
        "}\n";

    GLuint vertexShader =
        CompileShader(GL_VERTEX_SHADER, vertexSource);

    if (!vertexShader)
        return false;

    GLuint fragmentShader =
        CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (!fragmentShader)
    {
        glDeleteShader(vertexShader);

        return false;
    }


    Program = glCreateProgram();

    glAttachShader(Program, vertexShader);
    glAttachShader(Program, fragmentShader);

    glBindAttribLocation(Program, 0, "aPosition");
    glBindAttribLocation(Program, 1, "aTexCoord");

    glLinkProgram(Program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


    GLint linked = GL_FALSE;

    glGetProgramiv(
        Program,
        GL_LINK_STATUS,
        &linked
    );

    if (!linked)
    {
        char log[512];

        glGetProgramInfoLog(
            Program,
            sizeof(log),
            0,
            log
        );

        printf(
            "Text: shader link error: %s\n",
            log
        );

        glDeleteProgram(Program);
        Program = 0;

        return false;
    }


    AttribPosition = glGetAttribLocation(Program, "aPosition");
    AttribTexCoord = glGetAttribLocation(Program, "aTexCoord");
    UniformProjection = glGetUniformLocation(Program, "uProjection");
    UniformColor = glGetUniformLocation(Program, "uColor");
    UniformTexture = glGetUniformLocation(Program, "uTexture");


    glGenBuffers(1, &VBO);

    return true;
}


void Text::DestroyShader()
{
    if (VBO)
    {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }

    if (Program)
    {
        glDeleteProgram(Program);
        Program = 0;
    }
}


void Text::SetProjection(
    float width,
    float height
)
{
    // Orthographic projection, origin top-left, y down.
    // Equivalent to glOrtho(0, width, height, 0, -1, 1) but built
    // manually since GLES2 has no fixed-function matrix stack.

    memset(Projection, 0, sizeof(Projection));

    Projection[0] = 2.0f / width;
    Projection[5] = -2.0f / height;
    Projection[10] = -1.0f;
    Projection[12] = -1.0f;
    Projection[13] = 1.0f;
    Projection[15] = 1.0f;
}


void Text::SetColor(
    float r,
    float g,
    float b,
    float a
)
{
    ColorR = r;
    ColorG = g;
    ColorB = b;
    ColorA = a;
}


void Text::Unload()
{
    if (!Initialized)
        return;

    DestroyShader();

    if (Texture)
    {
        glDeleteTextures(1, &Texture);
        Texture = 0;
    }

    Initialized = false;
}


void Text::Draw(
    const char* text,
    float x,
    float y,
    float scale
)
{
    if (!Initialized)
        return;

    if (!text || !*text)
        return;


    // Each glyph becomes two triangles (6 vertices), each vertex is
    // { x, y, u, v }.
    std::vector<float> vertices;
    vertices.reserve(strlen(text) * 6 * 4);

    float penX = x;
    float penY = y;

    for (const unsigned char* p = (const unsigned char*)text; *p; p++)
    {
        unsigned char c = *p;

        if (c < FirstChar || c >= FirstChar + NumChars)
            continue;

        stbtt_aligned_quad q;

        // stbtt_GetBakedQuad advances (qx, qy) by this glyph's
        // advance width, starting from (0, 0), so afterwards
        // (qx, qy) IS the unscaled advance for this character.
        float qx = 0.0f;
        float qy = 0.0f;

        stbtt_GetBakedQuad(
            BakedChars,
            AtlasWidth,
            AtlasHeight,
            c - FirstChar,
            &qx,
            &qy,
            &q,
            1
        );

        float x0 = penX + q.x0 * scale;
        float y0 = penY + q.y0 * scale;
        float x1 = penX + q.x1 * scale;
        float y1 = penY + q.y1 * scale;

        // Triangle 1
        vertices.push_back(x0); vertices.push_back(y0);
        vertices.push_back(q.s0); vertices.push_back(q.t0);

        vertices.push_back(x1); vertices.push_back(y0);
        vertices.push_back(q.s1); vertices.push_back(q.t0);

        vertices.push_back(x1); vertices.push_back(y1);
        vertices.push_back(q.s1); vertices.push_back(q.t1);

        // Triangle 2
        vertices.push_back(x0); vertices.push_back(y0);
        vertices.push_back(q.s0); vertices.push_back(q.t0);

        vertices.push_back(x1); vertices.push_back(y1);
        vertices.push_back(q.s1); vertices.push_back(q.t1);

        vertices.push_back(x0); vertices.push_back(y1);
        vertices.push_back(q.s0); vertices.push_back(q.t1);

        penX += qx * scale;
        penY += qy * scale;
    }

    if (vertices.empty())
        return;


    // ------------------------------------------------------------
    // Draw
    // ------------------------------------------------------------

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(Program);

    glUniformMatrix4fv(UniformProjection, 1, GL_FALSE, Projection);
    glUniform4f(UniformColor, ColorR, ColorG, ColorB, ColorA);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);
    glUniform1i(UniformTexture, 0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(float),
        vertices.data(),
        GL_DYNAMIC_DRAW
    );

    glEnableVertexAttribArray(AttribPosition);
    glVertexAttribPointer(
        AttribPosition,
        2,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(AttribTexCoord);
    glVertexAttribPointer(
        AttribTexCoord,
        2,
        GL_FLOAT,
        GL_FALSE,
        4 * sizeof(float),
        (void*)(2 * sizeof(float))
    );

    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertices.size() / 4));

    glDisableVertexAttribArray(AttribPosition);
    glDisableVertexAttribArray(AttribTexCoord);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

}

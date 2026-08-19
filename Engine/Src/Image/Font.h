#pragma once
#define STB_TRUETYPE_IMPLEMENTATION
#include "Engine/dependencies/include.h"

namespace Absolut
{

class Text
{
public:
    Text();
    ~Text();

    // Loads a .ttf file and bakes an ASCII glyph atlas at the given
    // pixel size. Also creates the GLES2 shader/VBO used for drawing.
    bool LoadFont(
        const char* path,
        int size
    );

    void Unload();

    // Call once (and whenever the viewport changes) to set up the
    // orthographic projection text is drawn with. Origin is top-left,
    // y increases downward, matching typical screen/UI coordinates.
    void SetProjection(
        float width,
        float height
    );

    void Draw(
        const char* text,
        float x,
        float y,
        float scale = 1.0f
    );

    void SetColor(
        float r,
        float g,
        float b,
        float a = 1.0f
    );

private:
    bool CreateShader();
    void DestroyShader();
    GLuint CompileShader(
        GLenum type,
        const char* source
    );

    static const int FirstChar = 32;   // ' '
    static const int NumChars  = 96;   // 32..127

    stbtt_bakedchar BakedChars[NumChars];

    GLuint Texture;
    int AtlasWidth;
    int AtlasHeight;

    bool Initialized;

    float ColorR;
    float ColorG;
    float ColorB;
    float ColorA;

    GLuint Program;
    GLint  AttribPosition;
    GLint  AttribTexCoord;
    GLint  UniformProjection;
    GLint  UniformColor;
    GLint  UniformTexture;

    GLuint VBO;

    float Projection[16];
};

}

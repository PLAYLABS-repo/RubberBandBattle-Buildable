#define STB_IMAGE_IMPLEMENTATION
#include "Engine/dependencies/include.h"
#include "Texture.h"

namespace Absolut
{

Image::Image()
{
    Texture = 0;
    width = 0;
    height = 0;
}


bool Image::load(const char* path)
{
    int channels = 0;

    unsigned char* pixels = stbi_load(
        path,
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );

    if (!pixels)
    {
        printf(
            "Failed to load image: %s\n",
            stbi_failure_reason()
        );

        return false;
    }

    /*
        Premultiply alpha into RGB.

        Without this, any pixel that's fully or partly transparent
        keeps whatever RGB its source PNG happened to store there -
        very often white, since that's what image editors leave
        behind under erased/transparent areas. GL_LINEAR filtering
        then blends those "invisible" white texels into the visible
        edge texels next to them, and straight (GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA) blending has no way to discount that,
        so every sprite/quad edge picks up a faint white halo/tint.

        Premultiplying here means transparent texels store (0,0,0,0)
        instead of (255,255,255,0), so interpolating toward them
        fades color out instead of blending in white. This must be
        paired with GL_ONE / GL_ONE_MINUS_SRC_ALPHA blending wherever
        this texture is drawn (see Quad::draw, Mesh::draw,
        Window.h init, Anim.cpp) - do not mix premultiplied textures
        with GL_SRC_ALPHA blending or colors will be too dark.
    */

    for (int i = 0; i < width * height; ++i)
    {
        unsigned char* p = pixels + i * 4;
        float a = p[3] / 255.0f;

        p[0] = (unsigned char)(p[0] * a + 0.5f);
        p[1] = (unsigned char)(p[1] * a + 0.5f);
        p[2] = (unsigned char)(p[2] * a + 0.5f);
    }

    glGenTextures(1, &Texture);

    glBindTexture(
        GL_TEXTURE_2D,
        Texture
    );

    // Do NOT let OpenGL generate/change the image.
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

    /*
        stb_image gives us:

        R
        G
        B
        A

        exactly as 8-bit values.

        GLES2:
        internal format = GL_RGBA
        source format   = GL_RGBA
        source type     = GL_UNSIGNED_BYTE
    */

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
    );

    GLenum error = glGetError();

    if (error != GL_NO_ERROR)
    {
        printf(
            "glTexImage2D error: 0x%X\n",
            error
        );

        glBindTexture(
            GL_TEXTURE_2D,
            0
        );

        stbi_image_free(pixels);

        glDeleteTextures(
            1,
            &Texture
        );

        Texture = 0;

        width = 0;
        height = 0;

        return false;
    }

    stbi_image_free(pixels);

    glBindTexture(
        GL_TEXTURE_2D,
        0
    );

    return true;
}


void Image::Unload()
{
    if (Texture != 0)
    {
        glDeleteTextures(
            1,
            &Texture
        );

        Texture = 0;
    }

    width = 0;
    height = 0;
}

}

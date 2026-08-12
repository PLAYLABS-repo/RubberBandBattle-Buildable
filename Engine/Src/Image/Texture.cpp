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

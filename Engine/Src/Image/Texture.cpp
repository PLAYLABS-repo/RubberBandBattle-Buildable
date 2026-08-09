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
    int channels;

    unsigned char* pixels = stbi_load(
        path,
        &width,
        &height,
        &channels,
        4
    );

    if (!pixels)
    {
        printf(
            "Failed to load image: %s\n",
            stbi_failure_reason()
        );

        return false;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glGenTextures(1, &Texture);
    glBindTexture(GL_TEXTURE_2D, Texture);

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
        GL_CLAMP
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP
    );

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

    stbi_image_free(pixels);

    return true;
}

void Image::Unload()
{
    if (Texture != 0)
    {
        glDeleteTextures(1, &Texture);
        Texture = 0;
    }

    width = 0;
    height = 0;

}
}

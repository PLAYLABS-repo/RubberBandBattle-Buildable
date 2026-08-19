#include "Engine/dependencies/include.h"
#include "Texture.h"

#include <stdio.h>
#include <vector>

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
    // ------------------------------------------------------------
    // Find the image format
    // ------------------------------------------------------------

    FREE_IMAGE_FORMAT format =
        FreeImage_GetFileType(path, 0);

    if (format == FIF_UNKNOWN)
    {
        format =
            FreeImage_GetFIFFromFilename(path);
    }

    if (format == FIF_UNKNOWN)
    {
        printf(
            "Failed to determine image format: %s\n",
            path
        );

        return false;
    }


    // ------------------------------------------------------------
    // Load image
    // ------------------------------------------------------------

    FIBITMAP* bitmap =
        FreeImage_Load(format, path);

    if (!bitmap)
    {
        printf(
            "Failed to load image: %s\n",
            path
        );

        return false;
    }


    // ------------------------------------------------------------
    // Convert to 32-bit
    //
    // FreeImage 32-bit images are BGRA.
    // We convert them to RGBA for OpenGL.
    // ------------------------------------------------------------

    FIBITMAP* converted =
        FreeImage_ConvertTo32Bits(bitmap);

    FreeImage_Unload(bitmap);

    if (!converted)
    {
        printf(
            "Failed to convert image to 32-bit: %s\n",
            path
        );

        return false;
    }


    width =
        (int)FreeImage_GetWidth(converted);

    height =
        (int)FreeImage_GetHeight(converted);


    if (width <= 0 || height <= 0)
    {
        printf(
            "Invalid image dimensions: %s\n",
            path
        );

        FreeImage_Unload(converted);

        width = 0;
        height = 0;

        return false;
    }


    // ------------------------------------------------------------
    // FreeImage stores pixels bottom-to-top.
    //
    // We make a normal RGBA buffer for OpenGL.
    // ------------------------------------------------------------

    BYTE* bits =
        FreeImage_GetBits(converted);

    if (!bits)
    {
        printf(
            "Failed to get image pixels: %s\n",
            path
        );

        FreeImage_Unload(converted);

        width = 0;
        height = 0;

        return false;
    }


    std::vector<unsigned char> pixels(
        width * height * 4
    );


    unsigned int pitch =
        FreeImage_GetPitch(converted);


    // ------------------------------------------------------------
    // Convert BGRA -> RGBA
    // ------------------------------------------------------------

    for (int y = 0; y < height; ++y)
    {
        BYTE* source =
            bits + y * pitch;

        int dstY =
            height - 1 - y;

        unsigned char* destination =
            pixels.data() +
            dstY * width * 4;


        for (int x = 0; x < width; ++x)
        {
            BYTE* p =
                source + x * 4;

            unsigned char* out =
                destination + x * 4;


            // FreeImage = BGRA
            // OpenGL   = RGBA

            out[0] = p[FI_RGBA_RED];
            out[1] = p[FI_RGBA_GREEN];
            out[2] = p[FI_RGBA_BLUE];
            out[3] = p[FI_RGBA_ALPHA];
        }
    }


    FreeImage_Unload(converted);


    // ------------------------------------------------------------
    // Premultiply alpha
    // ------------------------------------------------------------

    for (int i = 0; i < width * height; ++i)
    {
        unsigned char* p =
            pixels.data() + i * 4;

        float a =
            p[3] / 255.0f;

        p[0] =
            (unsigned char)(p[0] * a + 0.5f);

        p[1] =
            (unsigned char)(p[1] * a + 0.5f);

        p[2] =
            (unsigned char)(p[2] * a + 0.5f);
    }


    // ------------------------------------------------------------
    // Create OpenGL texture
    // ------------------------------------------------------------

    glGenTextures(
        1,
        &Texture
    );

    glBindTexture(
        GL_TEXTURE_2D,
        Texture
    );


    // ------------------------------------------------------------
    // Texture filtering
    // ------------------------------------------------------------

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


    // ------------------------------------------------------------
    // Texture wrapping
    // ------------------------------------------------------------

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


    // ------------------------------------------------------------
    // Upload RGBA pixels
    // ------------------------------------------------------------

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels.data()
    );


    // ------------------------------------------------------------
    // Check OpenGL error
    // ------------------------------------------------------------

    GLenum error =
        glGetError();

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

        glDeleteTextures(
            1,
            &Texture
        );

        Texture = 0;

        width = 0;
        height = 0;

        return false;
    }


    // ------------------------------------------------------------
    // Done
    // ------------------------------------------------------------

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

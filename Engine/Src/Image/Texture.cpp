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
    if (!path)
        return false;

    // ------------------------------------------------------------
    // Find image format
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
        FreeImage_Load(
            format,
            path
        );

    if (!bitmap)
    {
        printf(
            "Failed to load image: %s\n",
            path
        );

        return false;
    }


    // ------------------------------------------------------------
    // Convert to 32-bit BGRA
    // ------------------------------------------------------------

    FIBITMAP* converted =
        FreeImage_ConvertTo32Bits(
            bitmap
        );

    FreeImage_Unload(
        bitmap
    );

    if (!converted)
    {
        printf(
            "Failed to convert image to 32-bit: %s\n",
            path
        );

        return false;
    }


    // ------------------------------------------------------------
    // Dimensions
    // ------------------------------------------------------------

    width =
        (int)FreeImage_GetWidth(
            converted
        );

    height =
        (int)FreeImage_GetHeight(
            converted
        );


    if (width <= 0 ||
        height <= 0)
    {
        printf(
            "Invalid image dimensions: %s\n",
            path
        );

        FreeImage_Unload(
            converted
        );

        width = 0;
        height = 0;

        return false;
    }


    // ------------------------------------------------------------
    // Get bits
    // ------------------------------------------------------------

    BYTE* bits =
        FreeImage_GetBits(
            converted
        );

    if (!bits)
    {
        printf(
            "Failed to get image pixels: %s\n",
            path
        );

        FreeImage_Unload(
            converted
        );

        width = 0;
        height = 0;

        return false;
    }


    std::vector<unsigned char> pixels(
        width * height * 4
    );


    unsigned int pitch =
        FreeImage_GetPitch(
            converted
        );


    // ------------------------------------------------------------
    // FreeImage BGRA bottom-up
    //
    // Convert to OpenGL-friendly RGBA top-down.
    //
    // IMPORTANT:
    // Do NOT premultiply alpha.
    // ------------------------------------------------------------

    for (int y = 0;
         y < height;
         ++y)
    {
        BYTE* source =
            bits +
            y * pitch;


        int dstY =
            height -
            1 -
            y;


        unsigned char* destination =
            pixels.data() +
            dstY *
            width *
            4;


        for (int x = 0;
             x < width;
             ++x)
        {
            BYTE* p =
                source +
                x * 4;


            unsigned char* out =
                destination +
                x * 4;


            // FreeImage -> RGBA

            out[0] =
                p[FI_RGBA_RED];

            out[1] =
                p[FI_RGBA_GREEN];

            out[2] =
                p[FI_RGBA_BLUE];

            out[3] =
                p[FI_RGBA_ALPHA];
        }
    }


    FreeImage_Unload(
        converted
    );


    // ------------------------------------------------------------
    // DEBUG: inspect first pixel
    // ------------------------------------------------------------

    printf(
        "Texture: %s\n",
        path
    );

    printf(
        "Size: %d x %d\n",
        width,
        height
    );

    printf(
        "First pixel RGBA: %u %u %u %u\n",
        (unsigned int)pixels[0],
        (unsigned int)pixels[1],
        (unsigned int)pixels[2],
        (unsigned int)pixels[3]
    );


    // ------------------------------------------------------------
    // Delete old texture if one exists
    // ------------------------------------------------------------

    if (Texture != 0)
    {
        glDeleteTextures(
            1,
            &Texture
        );

        Texture = 0;
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
    // Pixel unpack alignment
    // ------------------------------------------------------------

    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        1
    );


    // ------------------------------------------------------------
    // FILTERING
    //
    // NEAREST is intentional for this diagnostic.
    // It prevents neighboring atlas pixels from bleeding.
    // ------------------------------------------------------------

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_NEAREST
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_NEAREST
    );


    // ------------------------------------------------------------
    // WRAPPING
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
    // Upload exact RGBA bytes
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
    // Check upload
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
    // Unbind
    // ------------------------------------------------------------

    glBindTexture(
        GL_TEXTURE_2D,
        0
    );


    printf(
        "Texture loaded successfully.\n"
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

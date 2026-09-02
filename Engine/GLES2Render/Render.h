#pragma once

#include "Engine/dependencies/include.h"

namespace Absolut
{

inline void Clear(
    float r,
    float g,
    float b,
    float transparency
)
{
    glDisable(GL_SCISSOR_TEST);

    glColorMask(
        GL_TRUE,
        GL_TRUE,
        GL_TRUE,
        GL_TRUE
    );

    glClearColor(
        r,
        g,
        b,
        transparency
    );

    glClearDepthf(1.0f);

    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT
    );

    GLenum error = glGetError();

    if (error != GL_NO_ERROR)
    {
        printf(
            "Clear GL error: 0x%X\n",
            error
        );
    }
}

}


#pragma once
namespace Absolut{


class Camera{
public:
    Vec2 position = {0.0f, 0.0f};
    float zoom = 1.0f;
    float rotation = 0.0f; // optional but useful

    void apply(int screenWidth, int screenHeight);
};

inline void Camera::apply(int screenWidth, int screenHeight){
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float halfW = (screenWidth * 0.5f) / zoom;
    float halfH = (screenHeight * 0.5f) / zoom;

    glOrtho(
        position.x - halfW,
        position.x + halfW,
        position.y + halfH,
        position.y - halfH,
        -1.0f,
        1.0f
    );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // optional camera rotation (safe even if unused)
    if (rotation != 0.0f)
        glRotatef(rotation, 0, 0, 1);
}
}

#pragma once

namespace Absolut {
class Image {
public:
    Image();

    GLuint Texture;
    int width;
    int height;

    bool load(const char* path);
    void Unload();
    void UnloadAll();
};
}


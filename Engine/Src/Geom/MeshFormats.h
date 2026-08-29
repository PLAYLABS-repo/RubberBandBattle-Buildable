#pragma once
#include <string>

namespace Absolut {
class Mesh;

// Generic model loading entry point.
// FBX uses ufbx. OBJ/STL are handled by the small built-in readers.
bool LoadMeshFile(Mesh& mesh, const std::string& path);

// Loads an image file into a GL texture owned by the Mesh.
bool LoadMeshTexture(Mesh& mesh, const std::string& texturePath);
}

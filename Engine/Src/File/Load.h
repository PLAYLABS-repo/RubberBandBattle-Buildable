#pragma once
namespace Absolut {
std::string Load(const char* path)
{
    std::ifstream file(path);

    if (!file.is_open())
        return "";

    return std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
}
}

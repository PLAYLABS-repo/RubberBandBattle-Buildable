#include "Mesh.h"
#include "MeshFormats.h"

// Put ufbx.h/ufbx.c in the same dependency directory or adjust this include.
#include "ufbx.h"

#include <FreeImage.h>
#include <GLES2/gl2.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace Absolut {
namespace {

static std::string LowerExt(const std::string& path)
{
    size_t p = path.find_last_of(".");
    if (p == std::string::npos) return "";
    std::string e = path.substr(p);
    for (size_t i = 0; i < e.size(); ++i)
        e[i] = (char)std::tolower((unsigned char)e[i]);
    return e;
}

static GLuint TextureFromFile(const std::string& path)
{
    FREE_IMAGE_FORMAT fmt = FreeImage_GetFileType(path.c_str(), 0);
    if (fmt == FIF_UNKNOWN)
        fmt = FreeImage_GetFIFFromFilename(path.c_str());
    if (fmt == FIF_UNKNOWN) {
        printf("Mesh: unknown texture format: %s\n", path.c_str());
        return 0;
    }

    FIBITMAP* src = FreeImage_Load(fmt, path.c_str(), 0);
    if (!src) {
        printf("Mesh: failed to load texture: %s\n", path.c_str());
        return 0;
    }

    FIBITMAP* bmp = FreeImage_ConvertTo32Bits(src);
    FreeImage_Unload(src);
    if (!bmp) return 0;

    const int w = (int)FreeImage_GetWidth(bmp);
    const int h = (int)FreeImage_GetHeight(bmp);
    if (w <= 0 || h <= 0) {
        FreeImage_Unload(bmp);
        return 0;
    }

    std::vector<unsigned char> rgba((size_t)w * (size_t)h * 4);

    // FreeImage returns BGRA scanlines and its bitmap origin is bottom-left.
    // OpenGL textures are uploaded top-to-bottom here, so flip vertically.
    for (int y = 0; y < h; ++y) {
        const BYTE* srcRow = FreeImage_GetScanLine(bmp, h - 1 - y);
        unsigned char* dst = rgba.data() + (size_t)y * (size_t)w * 4;
        for (int x = 0; x < w; ++x) {
            const BYTE* p = srcRow + x * 4;
            dst[x * 4 + 0] = p[2];
            dst[x * 4 + 1] = p[1];
            dst[x * 4 + 2] = p[0];
            dst[x * 4 + 3] = p[3];
        }
    }
    FreeImage_Unload(bmp);

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

static void ResetMeshGeometry(Mesh& mesh)
{
    mesh.vertices.clear();
    mesh.indices.clear();
    mesh.skin.reset();
    mesh.bindPoseVertices.clear();
    mesh.skinVerts.clear();
    mesh.vertexBufferIsDynamic = false;
    mesh.verticesDirty = false;
}

static void AddTri(Mesh& mesh,
                   const Vec3& p0, const Vec3& p1, const Vec3& p2,
                   const Vec3& n0, const Vec3& n1, const Vec3& n2,
                   const Vec2& uv0, const Vec2& uv1, const Vec2& uv2)
{
    MeshVertex a{p0.x,p0.y,p0.z,n0.x,n0.y,n0.z,uv0.x,uv0.y};
    MeshVertex b{p1.x,p1.y,p1.z,n1.x,n1.y,n1.z,uv1.x,uv1.y};
    MeshVertex c{p2.x,p2.y,p2.z,n2.x,n2.y,n2.z,uv2.x,uv2.y};
    mesh.vertices.push_back(a);
    mesh.vertices.push_back(b);
    mesh.vertices.push_back(c);
}

static Vec3 Normalize3(const Vec3& v)
{
    float l = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
    if (l < 1e-8f) return Vec3{0,1,0};
    return Vec3{v.x/l, v.y/l, v.z/l};
}

static Vec3 Cross3(const Vec3& a, const Vec3& b)
{
    return Vec3{a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}

static Vec3 Sub3(const Vec3& a, const Vec3& b)
{
    return Vec3{a.x-b.x, a.y-b.y, a.z-b.z};
}

static int ObjIndex(int value, int count)
{
    if (value > 0) return value - 1;
    if (value < 0) return count + value;
    return -1;
}

static bool LoadOBJ(Mesh& mesh, const std::string& path)
{
    std::ifstream f(path.c_str());
    if (!f) {
        printf("Mesh::Load OBJ failed: %s\n", path.c_str());
        return false;
    }

    std::vector<Vec3> pos;
    std::vector<Vec3> nor;
    std::vector<Vec2> uv;
    std::string line;

    ResetMeshGeometry(mesh);

    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string tag;
        ss >> tag;

        if (tag == "v") {
            Vec3 p{}; ss >> p.x >> p.y >> p.z; pos.push_back(p);
        } else if (tag == "vn") {
            Vec3 n{}; ss >> n.x >> n.y >> n.z; nor.push_back(n);
        } else if (tag == "vt") {
            Vec2 t{}; ss >> t.x >> t.y; uv.push_back(t);
        } else if (tag == "f") {
            std::vector<std::string> refs;
            std::string r;
            while (ss >> r) refs.push_back(r);
            if (refs.size() < 3) continue;

            struct V { int p=-1,t=-1,n=-1; };
            std::vector<V> face(refs.size());
            for (size_t i=0;i<refs.size();++i) {
                int a=0,b=0,c=0;
                const std::string& s=refs[i];
                size_t p1=s.find('/');
                if (p1==std::string::npos) {
                    a=std::atoi(s.c_str());
                } else {
                    a=std::atoi(s.substr(0,p1).c_str());
                    size_t p2=s.find('/',p1+1);
                    if (p2==std::string::npos) b=std::atoi(s.substr(p1+1).c_str());
                    else {
                        if (p2>p1+1) b=std::atoi(s.substr(p1+1,p2-p1-1).c_str());
                        if (p2+1<s.size()) c=std::atoi(s.substr(p2+1).c_str());
                    }
                }
                face[i].p=ObjIndex(a,(int)pos.size());
                face[i].t=ObjIndex(b,(int)uv.size());
                face[i].n=ObjIndex(c,(int)nor.size());
            }

            // Fan triangulation: v0,v1,v2; v0,v2,v3; ...
            for (size_t i=1;i+1<face.size();++i) {
                V vv[3]={face[0],face[i],face[i+1]};
                if (vv[0].p<0 || vv[1].p<0 || vv[2].p<0) continue;
                Vec3 p0=pos[vv[0].p], p1=pos[vv[1].p], p2=pos[vv[2].p];
                Vec3 fn=Normalize3(Cross3(Sub3(p1,p0),Sub3(p2,p0)));
                Vec3 n0=vv[0].n>=0?nor[vv[0].n]:fn;
                Vec3 n1=vv[1].n>=0?nor[vv[1].n]:fn;
                Vec3 n2=vv[2].n>=0?nor[vv[2].n]:fn;
                Vec2 t0=vv[0].t>=0?uv[vv[0].t]:Vec2{0,0};
                Vec2 t1=vv[1].t>=0?uv[vv[1].t]:Vec2{1,0};
                Vec2 t2=vv[2].t>=0?uv[vv[2].t]:Vec2{0,1};
                AddTri(mesh,p0,p1,p2,n0,n1,n2,t0,t1,t2);
            }
        }
    }

    if (mesh.vertices.empty()) {
        printf("Mesh::Load OBJ: no triangles in %s\n", path.c_str());
        return false;
    }
    mesh.upload();
    return true;
}

static bool LoadFBX(Mesh& mesh, const std::string& path)
{
    ufbx_load_opts opts = {};
    opts.load_external_files = true;
    opts.ignore_missing_external_files = true;
    opts.generate_missing_normals = true;
    opts.target_axes = ufbx_axes_right_handed_y_up;
    opts.target_unit_meters = 1.0f;

    ufbx_error error = {};
    ufbx_scene* scene = ufbx_load_file(path.c_str(), &opts, &error);
    if (!scene) {
        printf("Mesh::Load FBX failed: %s\n", error.description.data);
        return false;
    }

    ufbx_mesh* found = nullptr;
    for (size_t i=0;i<scene->nodes.count;++i) {
        ufbx_node* node=scene->nodes.data[i];
        if (node && node->mesh) { found=node->mesh; break; }
    }
    if (!found && scene->meshes.count) found=scene->meshes.data[0];
    if (!found) {
        printf("Mesh::Load FBX: no mesh found in %s\n", path.c_str());
        ufbx_free_scene(scene);
        return false;
    }

    ResetMeshGeometry(mesh);
    const bool hasNormals = found->vertex_normal.exists;
    const bool hasUV = found->vertex_uv.exists;
    std::vector<uint32_t> tri( (size_t)std::max<uint32_t>(3, found->max_face_triangles * 3) );

    for (size_t fi=0; fi<found->faces.count; ++fi) {
        ufbx_face face=found->faces.data[fi];
        uint32_t ntri=ufbx_triangulate_face(tri.data(),tri.size(),found,face);
        for (uint32_t k=0;k<ntri*3;++k) {
            uint32_t ix=tri[k];
            ufbx_vec3 p=ufbx_get_vertex_vec3(&found->vertex_position,ix);
            ufbx_vec3 n=hasNormals?ufbx_get_vertex_vec3(&found->vertex_normal,ix):ufbx_vec3{0,1,0};
            ufbx_vec2 t=hasUV?ufbx_get_vertex_vec2(&found->vertex_uv,ix):ufbx_vec2{0,0};
            mesh.vertices.push_back(MeshVertex{
                (float)p.x,(float)p.y,(float)p.z,
                (float)n.x,(float)n.y,(float)n.z,
                (float)t.x,(float)t.y
            });
        }
    }

    ufbx_free_scene(scene);
    if (mesh.vertices.empty()) {
        printf("Mesh::Load FBX: no triangles in %s\n", path.c_str());
        return false;
    }
    mesh.upload();
    return true;
}

} // namespace

bool LoadMeshFile(Mesh& mesh, const std::string& path)
{
    const std::string ext=LowerExt(path);
    if (ext==".fbx") return LoadFBX(mesh,path);
    if (ext==".obj") return LoadOBJ(mesh,path);

    printf("Mesh::Load: unsupported format '%s'\n", ext.c_str());
    printf("Supported by this loader: .fbx .obj\n");
    return false;
}

bool LoadMeshTexture(Mesh& mesh, const std::string& texturePath)
{
    GLuint tex=TextureFromFile(texturePath);
    if (!tex) return false;

    if (mesh.ownsTexture && mesh.texture) {
        glDeleteTextures(1,&mesh.texture);
    }
    mesh.texture=tex;
    mesh.ownsTexture=true;
    return true;
}

} // namespace Absolut

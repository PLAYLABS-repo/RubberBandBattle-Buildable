#include "HelloInitializer.h"
#include "Engine/dependencies/engineincludes.h"
namespace Absolut {

void HelloWorldInit(){

Absolut::Camera SceneCamera;

   SceneCamera.position = {0.0f,0.0f};
   SceneCamera.zoom = 1.0f;
    auto lastTime = std::chrono::steady_clock::now();



    Absolut::Window ScenePreview;

    ScenePreview.create("ScenePreview", 1280, 720);




    typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);


    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT =
    (PFNWGLSWAPINTERVALEXTPROC)
    wglGetProcAddress("wglSwapIntervalEXT");
    wglSwapIntervalEXT(1);
    while (ScenePreview.update()){

    Absolut::Clear( 0.15f,0.15f,0.15f,1.0f);


    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    SceneCamera.apply(1280, 720);
//Do not change anything here, this is a scene initializer!
     ScenePreview.swap();
    }

}

int EndProcess(){
 return 0;
}

}




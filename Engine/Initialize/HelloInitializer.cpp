#include "HelloInitializer.h"

namespace Absolut {
  Window ScenePreview;
  Camera SceneCamera;
  auto lastTime = std::chrono::steady_clock::now();
  void HelloWorldInit(){


Absolut::Camera SceneCamera;


   SceneCamera.position = {0.0f,0.0f};
   SceneCamera.zoom = 1.0f;




    ScenePreview.create("ScenePreview", 1280, 720);


    ScenePreview.setVSync(true);

}
void SwapWindow(Camera& camera){
    camera.apply(1280, 720);
    ScenePreview.swap();
}


int EndProcess(){
 return 0;
}

}




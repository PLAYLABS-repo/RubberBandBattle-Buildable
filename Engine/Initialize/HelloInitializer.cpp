#include "HelloInitializer.h"

namespace Absolut {
  Window ScenePreview;
  Camera SceneCamera;

  void HelloWorldInit(){


Absolut::Camera SceneCamera;



extern Audio AudioSystem;
  AudioSystem.Init();
    ScenePreview.create("RubberBandBattle", 1280, 720);


    ScenePreview.setVSync(true);

}
void SwapWindow(Absolut::Camera& SceneCamera){
    SceneCamera.apply(1280, 720);
    ScenePreview.swap();
}


int EndProcess(){


 return 0;
}

}




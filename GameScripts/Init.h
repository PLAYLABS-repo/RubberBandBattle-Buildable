#include "Engine/dependencies/include.h"
namespace Absolut{

 inline  Absolut::Mesh myModel;
     Absolut::Text text;

     extern Absolut::Mesh cube = Absolut::Mesh::CreateCube(1.5f);
    inline bool InitAssets(){

text.LoadFont("Resources/Font/Confale.ttf", 32);   // path + pixel size
text.SetProjection(ScenePreview.getWidth(), ScenePreview.getHeight());
   Absolut::myModel.LoadFromGLTF("Resources/Mesh/Kiffer_Model_GM.glb");      // match your viewport
text.SetColor(1.0f, 1.0f, 1.0f, 1.0f);                // white, opaque


cube.position = {0, 0, 0};
cube.rotation.y = 30.0f;
cube.scale = {1, 1, 1};

          return true;
    }


}

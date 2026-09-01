#include "Engine/dependencies/include.h"
namespace Absolut{

  inline  Absolut::Mesh Player;
     Absolut::Text text;
      Absolut::Text DebugText;

     extern Absolut::Mesh cube = Absolut::Mesh::CreateCube(1.5f);
    inline bool InitAssets(){

text.LoadFont("Resources/Font/Confale.ttf", 32);   // path + pixel size
DebugText.LoadFont("Resources/Font/Confale.ttf", 32);   // path + pixel size
text.SetProjection(ScenePreview.getWidth(), ScenePreview.getHeight());
   Absolut::Player.LoadFromGLTF("Resources/Mesh/Kiffer_Model_GM.glb");      // match your viewport
  Absolut::Player.GLTFApplyTex( "Resources/Skins/KifferTeamRed.png");
Absolut::Player.position =
        Absolut::Vec3(0.0f, 0.0f, 0.0f);


        Absolut::SceneCamera.pitch = 20;
    Absolut::Player.useLighting = false;
text.SetColor(1.0f, 1.0f, 1.0f, 1.0f);                // white, opaque


cube.position = {0, 0, 0};
cube.rotation.y = 30.0f;
cube.scale = {1, 1, 1};

          return true;
    }


}

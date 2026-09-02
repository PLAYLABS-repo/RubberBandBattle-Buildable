#include "Engine/dependencies/include.h"
namespace RubberBandBattle{

  inline    Absolut::Mesh PlayerModel;
   Absolut::Text text;
   Absolut::Text DebugText;

    inline bool InitAssets(){

text.LoadFont("Resources/Font/Confale.ttf", 32);   // path + pixel size
DebugText.LoadFont("Resources/Font/Confale.ttf", 32);   // path + pixel size
text.SetProjection(Absolut::ScenePreview.getWidth(), Absolut::ScenePreview.getHeight());
 PlayerModel.LoadFromGLTF("Resources/Mesh/Kiffer_Model_GM.glb");      // match your viewport
 PlayerModel.GLTFApplyTex( "Resources/Skins/KifferTeamRed.png");
PlayerModel.position =
        Absolut::Vec3(0.0f, 0.0f, 0.0f);


        Absolut::SceneCamera.pitch = 20;
PlayerModel.useLighting = false;
text.SetColor(1.0f, 1.0f, 1.0f, 1.0f);                // white, opaque



          return true;
    }


}

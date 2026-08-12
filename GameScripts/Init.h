#include "Engine/dependencies/include.h"
namespace Absolut
{
    extern Image PlayerImg;
    extern Atlas PlayerMap;
    extern Animator PlayerAnim;


    inline bool InitAssets(){
          PlayerImg.load("Resources/Skins/spritemap.png");
          PlayerMap.load("Resources/Skins/spritemap.json");
          PlayerAnim.Load("Resources/Skins/Animation.json");
          return true;
    }

    inline bool FreeAssets(){
    PlayerAnim.FreeAnim();
    PlayerImg.Unload();
    FreeAtlas(&PlayerMap);
    }
}

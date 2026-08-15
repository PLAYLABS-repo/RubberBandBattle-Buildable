#include "Engine/dependencies/include.h"
namespace Absolut{

     Absolut::Mesh myModel;
     extern Absolut::Mesh cube = Absolut::Mesh::CreateCube(1.5f);
    inline bool InitAssets(){


         Absolut::InitAssets();

cube.position = {0, 0, 0};
cube.rotation.y = 30.0f;
cube.scale = {1, 1, 1};

          return true;
    }


}


#include "Engine/dependencies/include.h"

#pragma once
//PascalCase is better, fuck you if you prefer others
namespace Absolut{

 inline void Clear(float r, float g, float b, float transparency){
     glClearColor( r,  g , b, transparency);
     glClear(GL_COLOR_BUFFER_BIT);

 }

}

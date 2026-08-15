#pragma once

#include "Engine/dependencies/engineincludes.h"

namespace Absolut
{

extern Window ScenePreview;
extern Camera SceneCamera;

extern float currentTime;

extern Audio AudioSystem;
void HelloWorldInit();

void SwapWindow(Camera& camera);

int EndProcess();

}

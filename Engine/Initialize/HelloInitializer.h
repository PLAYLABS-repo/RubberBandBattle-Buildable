#pragma once

#include "Engine/dependencies/engineincludes.h"

namespace Absolut
{

extern Window ScenePreview;
extern Camera SceneCamera;

extern float currentTime;

void HelloWorldInit();

void SwapWindow(Camera& camera);

int EndProcess();

}

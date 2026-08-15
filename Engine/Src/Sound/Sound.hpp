#pragma once

#include "Engine/dependencies/miniaudio.h"

namespace Absolut
{

class Audio
{
public:
    Audio();
    ~Audio();

    bool Init();

    void PlayOnce(const char* filename);
    void Loop(const char* filename);
    void Play(const char* filename, int repeatTimes);

    void StopLoop();

    void Shutdown();

private:
    ma_engine engine;

    ma_sound loopSound;
    bool loopSoundInitialized;

    bool initialized;
};

extern Audio AudioSystem;

}

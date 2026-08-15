#define MINIAUDIO_IMPLEMENTATION
#include "Engine/dependencies/miniaudio.h"

#include "Sound.hpp"

namespace Absolut
{

Audio AudioSystem;

Audio::Audio()
    : loopSoundInitialized(false),
      initialized(false)
{
}

Audio::~Audio()
{
    Shutdown();
}

bool Audio::Init()
{
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS)
        return false;

    initialized = true;

    return true;
}

void Audio::PlayOnce(const char* filename)
{
    if (!initialized)
        return;

    ma_engine_play_sound(
        &engine,
        filename,
        NULL
    );
}

void Audio::Loop(const char* filename)
{
    if (!initialized)
        return;

    // Stop and clean up previous loop.
    if (loopSoundInitialized)
    {
        ma_sound_uninit(&loopSound);
        loopSoundInitialized = false;
    }

    if (ma_sound_init_from_file(
            &engine,
            filename,
            0,
            NULL,
            NULL,
            &loopSound) != MA_SUCCESS)
    {
        return;
    }

    loopSoundInitialized = true;

    ma_sound_set_looping(
        &loopSound,
        MA_TRUE
    );

    ma_sound_start(
        &loopSound
    );
}

void Audio::StopLoop()
{
    if (!loopSoundInitialized)
        return;

    ma_sound_stop(&loopSound);

    ma_sound_uninit(&loopSound);

    loopSoundInitialized = false;
}

void Audio::Play(const char* filename, int repeatTimes)
{
    if (!initialized || repeatTimes <= 0)
        return;

    /*
        For now, repeatTimes == 1 can use PlayOnce().
        Proper asynchronous N-times playback should use
        a persistent ma_sound and an end callback.
    */

    if (repeatTimes == 1)
    {
        PlayOnce(filename);
        return;
    }

    // TODO: asynchronous repeat implementation
}

void Audio::Shutdown()
{
    if (!initialized)
        return;

    StopLoop();

    ma_engine_uninit(&engine);

    initialized = false;
}

}

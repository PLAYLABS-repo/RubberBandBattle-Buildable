#include "Engine/Initialize/HelloInitializer.h"
#include "GameScripts/Init.h"
#include "Engine/dependencies/engineincludes.h"
#include <chrono>
#include <cmath>

int main()
{
    Absolut::HelloWorldInit();

    // --- one-time asset setup (font, cube, etc.) ---
    if (!Absolut::InitAssets())
    {
        printf("FAILED to init assets!\n");
    }

    // --- FPS mouse look setup ---
    // NOTE: adjust getHWND() to whatever your ScenePreview actually
    // exposes if it's named differently.
    Absolut::Mouse::Get().BindWindow(Absolut::ScenePreview.getHandle());
    Absolut::Mouse::Get().EnableFPSMode(true);

    Absolut::SceneCamera.mode = Absolut::ProjectionMode::Perspective;
    Absolut::SceneCamera.position = { 0.0f, 0.0f, 10.0f };
    Absolut::SceneCamera.perspNear = 0.1f;
    Absolut::SceneCamera.perspFar  = 1000.0f;

    Absolut::cube.r = 0.8f; Absolut::cube.g = 0.2f; Absolut::cube.b = 0.2f;




    Absolut::myModel.position = {0.0f, 0.0f, 0.0f};
    Absolut::myModel.rotation = {0.0f, 0.0f, 0.0f};
    Absolut::myModel.scale    = {1.0f, 1.0f, 1.0f};
    Absolut::myModel.useLighting = true;

    // --- start playing an animation, if the model has one ---
    if (Absolut::myModel.GetAnimationCount() > 0)
    {
        printf("Playing: %s\n", Absolut::myModel.GetAnimationName(1).c_str());
        Absolut::myModel.SetAnimation(1, true);
 // index 0, looping
    }

    Absolut::AudioSystem.Loop("Resources/Sound/BackgroundMusic/loading.mp3");

    // --- for delta time ---
    auto lastTime = std::chrono::steady_clock::now();

    bool mouseLocked = true; // matches EnableFPSMode(true) above

    while (Absolut::ScenePreview.update())
    {
        // --- compute this frame's delta time in seconds ---
        auto now = std::chrono::steady_clock::now();
        float deltaSeconds = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        Absolut::Clear(0.15f, 0.15f, 0.15f, 1.0f);
        Absolut::SceneCamera.apply(1280, 720);

        Absolut::cube.draw();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        // ------------------------------------------------------
        // MOUSE LOOK (yaw/pitch from mouse delta)
        // ------------------------------------------------------

        Absolut::Mouse::Get().Update();

        float mouseSensitivity = 0.15f;

        Absolut::SceneCamera.yaw   += Absolut::Mouse::Get().deltaX * mouseSensitivity;
        Absolut::SceneCamera.pitch += Absolut::Mouse::Get().deltaY * mouseSensitivity;

        if (Absolut::SceneCamera.pitch > 89.0f)  Absolut::SceneCamera.pitch = 89.0f;
        if (Absolut::SceneCamera.pitch < -89.0f) Absolut::SceneCamera.pitch = -89.0f;

        // ------------------------------------------------------
        // MOVEMENT (tangent-relative WASD - moves relative to
        // where the camera is currently facing, driven by yaw)
        // ------------------------------------------------------

        float yawRad = Absolut::SceneCamera.yaw * 3.14159265f / 180.0f;

        Absolut::Vec3 forward = { sinf(yawRad), 0.0f, -cosf(yawRad) };
        Absolut::Vec3 right   = { cosf(yawRad), 0.0f,  sinf(yawRad) };

        float moveSpeed = 3.0f * deltaSeconds; // framerate-independent

        if (Absolut::KeyDown('W'))
        {
            Absolut::SceneCamera.position.x += forward.x * moveSpeed;
            Absolut::SceneCamera.position.z += forward.z * moveSpeed;
        }
        if (Absolut::KeyDown('S'))
        {
            Absolut::SceneCamera.position.x -= forward.x * moveSpeed;
            Absolut::SceneCamera.position.z -= forward.z * moveSpeed;
        }
        if (Absolut::KeyDown('A'))
        {
            Absolut::SceneCamera.position.x -= right.x * moveSpeed;
            Absolut::SceneCamera.position.z -= right.z * moveSpeed;
        }
        if (Absolut::KeyDown('D'))
        {
            Absolut::SceneCamera.position.x += right.x * moveSpeed;
            Absolut::SceneCamera.position.z += right.z * moveSpeed;
        }

        if (Absolut::KeyDown(VK_SPACE)) { Absolut::SceneCamera.position.y +=  moveSpeed; }
        if (Absolut::KeyDown(VK_SHIFT)) { Absolut::SceneCamera.position.y += -moveSpeed; }

        // ------------------------------------------------------
        // ESC releases the mouse from FPS look mode (shows the
        // cursor, stops recentering/deltas). Clicking the window
        // again re-locks it below.
        // ------------------------------------------------------

        if (Absolut::KeyDown(VK_ESCAPE) && mouseLocked)
        {
            Absolut::Mouse::Get().EnableFPSMode(false);
            mouseLocked = false;
        }

        if (Absolut::Mouse::Get().isLDown && !mouseLocked)
        {
            Absolut::Mouse::Get().EnableFPSMode(true);
            mouseLocked = true;
        }

        // --- advance the animation before drawing ---
        Absolut::myModel.UpdateAnimation(deltaSeconds);
        Absolut::myModel.draw();

        // ------------------------------------------------------
        // TEXT (drawn last, on top of everything, no depth test)
        // ------------------------------------------------------

        Absolut::text.SetProjection(Absolut::ScenePreview.getWidth(), Absolut::ScenePreview.getHeight());
        Absolut::text.Draw("Hello, world!", 50.0f, 100.0f, 1.0f);

        glEnable(GL_DEPTH_TEST);

        Absolut::SwapWindow(Absolut::SceneCamera);

        Absolut::cube.rotation.x += 1;
        Absolut::myModel.rotation.y += 1.0f;
    }

    Absolut::EndProcess();
    Absolut::text.Unload();

    return 0;
}

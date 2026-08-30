
#include "Engine/Initialize/HelloInitializer.h"
#include "GameScripts/Init.h"
#include "Engine/dependencies/engineincludes.h"
#include "GameScripts/RenderHandling/GroundPlane.h"
#include "GameScripts/Entity/Player/Player.h"
#include <chrono>
#include <cmath>
#include <cstdio>

int main()
{
    // ============================================================
    // ENGINE INITIALIZATION
    // ============================================================

    Absolut::HelloWorldInit();

    if (!Absolut::InitAssets())
    {
        printf("FAILED TO INIT ASSETS!\n");
        return 1;
    }

    printf("Assets initialized.\n");

    // ============================================================
    // WINDOW / MOUSE
    // ============================================================

    Absolut::Mouse::Get().BindWindow(
        Absolut::ScenePreview.getHandle()
    );

    Absolut::Mouse::Get().EnableFPSMode(true);

    // ============================================================
    // CAMERA
    // ============================================================

    Absolut::SceneCamera.mode =
        Absolut::ProjectionMode::Perspective;
Player Player;

    Absolut::SceneCamera.perspNear = 0.1f;
    Absolut::SceneCamera.perspFar  = 1000.0f;

    // ============================================================
    // CUBE
    // ============================================================

    Absolut::cube.position =
        Absolut::Vec3(-2.0f, 0.0f, 0.0f);

    Absolut::cube.rotation =
        Absolut::Vec3(0.0f, 0.0f, 0.0f);

    Absolut::cube.scale =
        Absolut::Vec3(1.0f, 1.0f, 1.0f);

    Absolut::cube.r = 0.8f;
    Absolut::cube.g = 0.2f;
    Absolut::cube.b = 0.2f;
    Absolut::Mesh ground = Absolut::Mesh::CreateCube(1.0f);
ground.CreateCube(1.0f);
ground.position = {0,0,0};
ground.scale = {100, 0, 100};
ground.r = 0.0f;ground.g = 1.0f;ground.b = 0.0f;
    Absolut::cube.useLighting = true;

    // ============================================================
    // FBX MODEL
    // ============================================================

    Absolut::myModel.position =
        Absolut::Vec3(0.0f, 0.0f, 0.0f);

    Absolut::myModel.rotation =
        Absolut::Vec3(0.0f, 0.0f, 0.0f);

    Absolut::myModel.scale =
        Absolut::Vec3(1.0f, 1.0f, 1.0f);

    Absolut::myModel.useLighting = false;

    // Leave culling OFF while testing the FBX.
    // This prevents an incorrectly-wound FBX from disappearing.


    // ============================================================
    // ANIMATIONS
    // ============================================================

    int animationCount =
        Absolut::myModel.GetAnimationCount();

    printf("FBX animation count: %d\n", animationCount);



    // NEVER use index 5 unless there are at least 6 animations.
    if (animationCount > 0)
    {
        printf(
            "Playing animation: %s\n",
            Absolut::myModel.GetAnimationName(0).c_str()
        );

        if (!Absolut::myModel.SetAnimation(0, true))
        {
            //none
        }
    }

    // ============================================================
    // AUDIO
    // ============================================================

    Absolut::AudioSystem.Loop(
        "Resources/Sound/BackgroundMusic/EnterGame.mp3"
    );

    // ============================================================
    // DELTA TIME
    // ============================================================

    auto lastTime =
        std::chrono::steady_clock::now();

    bool mouseLocked = true;

    // ============================================================
    // MAIN LOOP
    // ============================================================

    while (Absolut::ScenePreview.update())
    {

Absolut::SceneCamera.position =
        Absolut::Vec3(Player.position.x ,Player.position.y + 4.0f, Player.position.z + 7.5f);
        Absolut::SceneCamera.pitch = 20;
        // --------------------------------------------------------
        // DELTA TIME
        // --------------------------------------------------------

        auto now =
            std::chrono::steady_clock::now();

        float deltaSeconds =
            std::chrono::duration<float>(
                now - lastTime
            ).count();

        lastTime = now;

        // Prevent a huge timestep after dragging/debugging.
        if (deltaSeconds > 0.1f)
            deltaSeconds = 0.1f;

        // --------------------------------------------------------
        // CLEAR
        // --------------------------------------------------------

        Absolut::Clear(
            0.15f,
            0.15f,
            0.15f,
            1.0f
        );

        // --------------------------------------------------------
        // CAMERA PROJECTION
        // --------------------------------------------------------

        Absolut::SceneCamera.apply(
            Absolut::ScenePreview.getWidth(),
            Absolut::ScenePreview.getHeight()
        );

        // --------------------------------------------------------
        // MOUSE
        // --------------------------------------------------------

        //Absolut::Mouse::Get().Update();



         Player.UpdatePlayer(deltaSeconds);
           Absolut::myModel.position = Player.position;


        Absolut::myModel.UpdateAnimation(
            deltaSeconds
        );

        // --------------------------------------------------------
        // DRAW 3D
        // --------------------------------------------------------

        // Cube
        Absolut::cube.draw();

        // FBX MODEL
        Absolut::myModel.draw();
      ground.draw();
        // --------------------------------------------------------
        // RESTORE 2D STATE FOR TEXT
        // --------------------------------------------------------

        glDisable(GL_DEPTH_TEST); //Add 2d objects after this
        glDisable(GL_CULL_FACE);

        Absolut::text.SetProjection(
            Absolut::ScenePreview.getWidth(),
            Absolut::ScenePreview.getHeight()
        );

        Absolut::text.Draw(
            "Hello, world!",
            50.0f,
            100.0f,
            1.0f
        );

        // --------------------------------------------------------
        // RESTORE DEPTH
        // --------------------------------------------------------

        glEnable(GL_DEPTH_TEST);

        // --------------------------------------------------------
        // SWAP
        // --------------------------------------------------------

        Absolut::SwapWindow(
            Absolut::SceneCamera
        );

        // --------------------------------------------------------
        // TEST ROTATION
        // --------------------------------------------------------

        Absolut::cube.rotation.y +=
            60.0f * deltaSeconds;


    }

    // ============================================================
    // SHUTDOWN
    // ============================================================

    Absolut::text.Unload();

    Absolut::EndProcess();

    return 0;
}


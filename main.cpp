
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
RubberBandBattle::Player Player;


  RubberBandBattle::GroundInit(150, 150);

    // ============================================================
    // FBX MODEL
    // ============================================================



    // Leave culling OFF while testing the FBX.
    // This prevents an incorrectly-wound FBX from disappearing.


    // ============================================================
    // ANIMATIONS
    // ============================================================

    int animationCount =
        Absolut::Player.GetAnimationCount();

    printf("FBX animation count: %d\n", animationCount);



    // NEVER use index 5 unless there are at least 6 animations.
    if (animationCount > 0)
    {
        printf(
            "Playing animation: %s\n",
            Absolut::Player.GetAnimationName(0).c_str()
        );

        if (!Absolut::Player.SetAnimation(0, true))
        {
            //none
        }
    }

    // ============================================================
    // AUDIO
    // ============================================================

    Absolut::AudioSystem.Loop(
        "Resources/Sound/BackgroundMusic/LoadingActionBG.wav"
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
           Absolut::Player.position = Player.position;


        Absolut::Player.UpdateAnimation(
            deltaSeconds
        );

        // --------------------------------------------------------
        // DRAW 3D
        // --------------------------------------------------------

        // Cube


        // FBX MODEL
        Absolut::Player.draw();
       RubberBandBattle::GroundDraw();
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


    }

    // ============================================================
    // SHUTDOWN
    // ============================================================

    Absolut::text.Unload();

    Absolut::EndProcess();

    return 0;
}


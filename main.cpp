
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



    Absolut::AudioSystem.Loop(
        "Resources/Sound/BackgroundMusic/LoadingActionBG.wav"
    );



    auto lastTime =
        std::chrono::steady_clock::now();

    bool mouseLocked = true;

    while (Absolut::ScenePreview.update())
    {


Absolut::SceneCamera.position =
        Absolut::Vec3(Player.position.x ,Player.position.y + 4.0f, Player.position.z + 7.5f);

        auto now =
            std::chrono::steady_clock::now();

        float deltaSeconds =
            std::chrono::duration<float>(
                now - lastTime
            ).count();

        lastTime = now;

        if (deltaSeconds > 0.1f)
            deltaSeconds = 0.1f;

        Absolut::Clear(
            0.15f,
            0.15f,
            0.15f,
            1.0f
        );

        Absolut::SceneCamera.apply(
            Absolut::ScenePreview.getWidth(),
            Absolut::ScenePreview.getHeight()
        );



 if(Absolut::KeyPressed('P')){
         Player.TakeDamage(10);
    }
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

 Absolut::DebugText.SetProjection(
            Absolut::ScenePreview.getWidth(),
            Absolut::ScenePreview.getHeight()
        );
       Absolut::DebugText.Draw(
 ("Health: " + std::to_string(Player.Health)).c_str(),
    50.0f,
    100.0f,
    1.0f
);
Absolut::DebugText.Draw(
 "The player model is temporary.",
    50.0f,
    150.0f,
    1.0f
);
Absolut::DebugText.Draw(
 "More features will come soon when Absolut-Engine-ULTRA is updated.",
    50.0f,
    200.0f,
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

    Absolut::DebugText.Unload();


    Absolut::EndProcess();

    return 0;
}


#include "Engine/Initialize/HelloInitializer.h"
#include "Init.h"
#include "Engine/dependencies/engineincludes.h"
#include "Environment/GroundPlane.h"
#include "GameScripts/Entity/Player/Player.h"

#include <chrono>
#include <string>

namespace RubberBandBattle
{

enum BrawlMode
{
    BRAWL_MODE_DEBUG,
    BRAWL_MODE_GAME,
    BRAWL_MODE_FREE
};

void RunBrawlApp(BrawlMode mode)
{
    RubberBandBattle::Player Player;

    // ============================================================
    // INITIALIZATION
    // ============================================================

    Absolut::HelloWorldInit();

    RubberBandBattle::InitAssets();

    Absolut::Mouse::Get().BindWindow(
        Absolut::ScenePreview.getHandle()
    );

    Absolut::SceneCamera.mode =
        Absolut::ProjectionMode::Perspective;

GroundInit(150, 150);

    Absolut::AudioSystem.Loop(
        "Resources/Sound/BackgroundMusic/LoadingActionBG.wav"
    );

    auto lastTime = std::chrono::steady_clock::now();

    // ============================================================
    // MAIN LOOP
    // ============================================================

    while (Absolut::ScenePreview.update())
    {
        // --------------------------------------------------------
        // DELTA TIME
        // --------------------------------------------------------

        auto now = std::chrono::steady_clock::now();

        float deltaSeconds =
            std::chrono::duration<float>(
                now - lastTime
            ).count();

        lastTime = now;

        if (deltaSeconds > 0.1f)
            deltaSeconds = 0.1f;

        // --------------------------------------------------------
        // CAMERA
        // --------------------------------------------------------

        Absolut::SceneCamera.position =
            Absolut::Vec3(
                Player.position.x,
                Player.position.y + 4.0f,
                Player.position.z + 7.5f
            );

        // --------------------------------------------------------
        // CLEAR
        // --------------------------------------------------------

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

        // ========================================================
        // APPLICATION MODE
        // ========================================================

        switch (mode)
        {
            // ====================================================
            // DEBUG
            // ====================================================

          case BRAWL_MODE_DEBUG:
{
    // --------------------------------------------------------
    // TEST DAMAGE
    // --------------------------------------------------------

    if (Absolut::KeyPressed('P'))
    {
        Player.TakeDamage(10);
        printf("Health: %d\n", Player.Health);
    }

    // --------------------------------------------------------
    // PLAYER UPDATE
    // --------------------------------------------------------

    Player.UpdatePlayer(deltaSeconds);

    // --------------------------------------------------------
    // APPLY PLAYER ANIMATION
    // --------------------------------------------------------

    static int appliedAnimation = -1;

    if (Player.lastAnimation != appliedAnimation)
    {
        bool looping =
            Player.lastAnimation != 1 &&
            Player.lastAnimation != 3;

        RubberBandBattle::PlayerModel.SetAnimation(
            Player.lastAnimation,
            looping
        );

        appliedAnimation = Player.lastAnimation;
    }

    // --------------------------------------------------------
    // PLAYER TRANSFORM
    // --------------------------------------------------------

    RubberBandBattle::PlayerModel.position =
        Player.position;

    RubberBandBattle::PlayerModel.rotation.y =
        Player.rotationY;

    // --------------------------------------------------------
    // ADVANCE ANIMATION
    // --------------------------------------------------------

    RubberBandBattle::PlayerModel.UpdateAnimation(
        deltaSeconds
    );

    // --------------------------------------------------------
    // DRAW PLAYER
    // --------------------------------------------------------

    RubberBandBattle::PlayerModel.draw();

    // --------------------------------------------------------
    // DRAW GROUND
    // --------------------------------------------------------

    GroundDraw();

    break;
}
            // ====================================================
            // GAME
            // ====================================================

            case BRAWL_MODE_GAME:
            {
                Player.UpdatePlayer(deltaSeconds);

                RubberBandBattle::PlayerModel.position =
                    Player.position;

                RubberBandBattle::PlayerModel.UpdateAnimation(
                    deltaSeconds
                );

                RubberBandBattle::PlayerModel.draw();

            GroundDraw();

                break;
            }

            // ====================================================
            // FREE
            // ====================================================


        }

        // ========================================================
        // 2D UI
        // ========================================================

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        RubberBandBattle::text.SetProjection(
            Absolut::ScenePreview.getWidth(),
            Absolut::ScenePreview.getHeight()
        );

        RubberBandBattle::DebugText.SetProjection(
            Absolut::ScenePreview.getWidth(),
            Absolut::ScenePreview.getHeight()
        );

        RubberBandBattle::DebugText.Draw(
            ("Health: " +
             std::to_string(Player.Health)).c_str(),
            50.0f,
            100.0f,
            1.0f
        );

        RubberBandBattle::DebugText.Draw(
            "The player model is temporary.",
            50.0f,
            150.0f,
            1.0f
        );

        RubberBandBattle::DebugText.Draw(
            "More features will come soon when Absolut-Engine-ULTRA is updated.",
            50.0f,
            200.0f,
            1.0f
        );


 if (Absolut::KeyPressed('P')){

    Player.Health -= 10.0f;
    printf("Health diminished \n");
}

        // ========================================================
        // RESTORE DEPTH
        // ========================================================

        glEnable(GL_DEPTH_TEST);

        // ========================================================
        // SWAP
        // ========================================================

        Absolut::SwapWindow(
            Absolut::SceneCamera
        );
    }

    // ============================================================
    // SHUTDOWN
    // ============================================================

    RubberBandBattle::text.Unload();
    RubberBandBattle::DebugText.Unload();

    Absolut::EndProcess();
}

} // namespace RubberBandBattle



#include "Engine/Initialize/HelloInitializer.h"
#include "GameScripts/Init.h"
#include <chrono>

namespace Absolut
{

enum class PlayerAnimState
{
    IDLE,
    WALK
};

Image PlayerImg;
Atlas PlayerMap;
Animator PlayerAnim;

PlayerAnimState LastPlayerAnimState =
    PlayerAnimState::IDLE;

}


int main()
{
    Absolut::HelloWorldInit();

    Absolut::InitAssets();


    // =========================================================
    // PLAYER POSITION
    // =========================================================

    Absolut::PlayerAnim.Parent.Enabled = true;

    Absolut::PlayerAnim.Parent.Position =
        Absolut::Vec2(640.0f, 360.0f);

    Absolut::PlayerAnim.Parent.Rotation =
        0.0f;

    Absolut::PlayerAnim.Parent.Scale =
        Absolut::Vec2(1.0f, 1.0f);




    Absolut::PlayerAnim.PlayLoopAnim(
        "PLAYER",
        "IDLE"
    );

    Absolut::LastPlayerAnimState =
        Absolut::PlayerAnimState::IDLE;


    // =========================================================
    // MAIN LOOP
    // =========================================================

    while (Absolut::ScenePreview.update())
    {
        Absolut::Clear(
            0.15f,
            0.15f,
            0.15f,
            1.0f
        );


        // =====================================================
        // INPUT
        // =====================================================

        bool moving =
            Absolut::KeyDown(VK_LEFT);


        // =====================================================
        // ANIMATION STATE
        // =====================================================

        if (
            moving &&
            Absolut::LastPlayerAnimState !=
                Absolut::PlayerAnimState::WALK
        )
        {
            Absolut::PlayerAnim.PlayLoopAnim(
                "PLAYER",
                "WALK"
            );

            Absolut::LastPlayerAnimState =
                Absolut::PlayerAnimState::WALK;
        }
        else if (
            !moving &&
            Absolut::LastPlayerAnimState !=
                Absolut::PlayerAnimState::IDLE
        )
        {
            Absolut::PlayerAnim.PlayLoopAnim(
                "PLAYER",
                "IDLE"
            );

            Absolut::LastPlayerAnimState =
                Absolut::PlayerAnimState::IDLE;
        }


        // =====================================================
        // UPDATE
        // =====================================================

        Absolut::PlayerAnim.Update(
            0.015f
        );


        // =====================================================
        // DRAW PLAYER
        // =====================================================

        Absolut::PlayerAnim.Draw(
            &Absolut::PlayerImg,
            &Absolut::PlayerMap,
            Absolut::SceneCamera
        );


        // =====================================================
        // CAMERA / PRESENT
        // =====================================================

        Absolut::SceneCamera.apply(
            1280,
            720
        );

        Absolut::SwapWindow(
            Absolut::SceneCamera
        );
    }


    // =========================================================
    // CLEANUP
    // =========================================================

    Absolut::FreeAssets();

    Absolut::EndProcess();

    return 0;
}

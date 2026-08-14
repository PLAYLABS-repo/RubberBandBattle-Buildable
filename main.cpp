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
Absolut::Mesh cube = Absolut::Mesh::CreateCube(1.5f);
Absolut::SceneCamera.mode = Absolut::ProjectionMode::Perspective;
Absolut::SceneCamera.position = { 0.0f, 0.0f, 10.0f };  // must be > object's z + perspNear
Absolut::SceneCamera.perspNear = 0.1f;
Absolut::SceneCamera.perspFar  = 1000.0f;
    // =========================================================
    // PLAYER POSITION
    // =========================================================

 Absolut::PlayerAnim.AnchorTo(Absolut::WORLD);

    Absolut::PlayerAnim.Parent.Enabled = true;

    Absolut::PlayerAnim.Parent.Position =
        Absolut::Vec2(0.0f, -100.0f);

    Absolut::PlayerAnim.Parent.Rotation =
        0.0f;

    Absolut::PlayerAnim.Parent.Scale =
        Absolut::Vec2(0.01f, 0.01f);



    Absolut::PlayerAnim.PlayLoopAnim(
        "PLAYER",
        "IDLE"
    );

    Absolut::LastPlayerAnimState =
        Absolut::PlayerAnimState::IDLE;

cube.position = {0, 0, 0};
cube.rotation.y = 30.0f;      // degrees
cube.scale = {1, 1, 1};
cube.r = 0.8f; cube.g = 0.2f; cube.b = 0.2f; // used when no texture

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
        // CAMERA
        // =====================================================

        Absolut::SceneCamera.apply(
            1280,
            720
        );


        // =====================================================
        // DRAW PLAYER
        // =====================================================

     cube.draw();
     glDisable(GL_DEPTH_TEST);
        Absolut::PlayerAnim.Draw(
            &Absolut::PlayerImg,
            &Absolut::PlayerMap,
            Absolut::SceneCamera
        );

          if (Absolut::KeyDown('W')){

          Absolut::SceneCamera.position.z += -0.1f;

    }
     if (Absolut::KeyDown('S')){

          Absolut::SceneCamera.position.z += 0.1f;

    }
       if (Absolut::KeyDown('A')){

          Absolut::SceneCamera.position.x += -0.1f;

    }
        if (Absolut::KeyDown('D')){

          Absolut::SceneCamera.position.x += 0.1f;

    }



        // =====================================================
        // PRESENT
        // =====================================================

        Absolut::SwapWindow(
            Absolut::SceneCamera
        );
           cube.rotation.x += 5;
    }

    // =========================================================
    // CLEANUP
    // =========================================================

    Absolut::FreeAssets();

    Absolut::EndProcess();

    return 0;
}

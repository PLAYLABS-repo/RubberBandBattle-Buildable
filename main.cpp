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

Absolut::Mesh myModel;
    Absolut::InitAssets();
Absolut::Mesh cube = Absolut::Mesh::CreateCube(1.5f);
Absolut::SceneCamera.mode = Absolut::ProjectionMode::Perspective;
Absolut::SceneCamera.position = { 0.0f, 0.0f, 10.0f };  // must be > object's z + perspNear
Absolut::SceneCamera.perspNear = 0.1f;
Absolut::SceneCamera.perspFar  = 1000.0f;


 Absolut::PlayerAnim.AnchorTo(Absolut::SCREEN);

    Absolut::PlayerAnim.Parent.Enabled = true;

    Absolut::PlayerAnim.Parent.Position =
        Absolut::Vec2(90.0f, 50.0f);

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

cube.position = {0, 0, 0};
cube.rotation.y = 30.0f;
cube.scale = {1, 1, 1};
cube.r = 0.8f; cube.g = 0.2f; cube.b = 0.2f;
myModel.LoadFromGLTF("Resources/Mesh/kiffer.glb");
myModel.position = {0.0f, 0.0f, 0.0f};
myModel.rotation = {0.0f, 0.0f, 0.0f};
myModel.scale    = {1.0f, 1.0f, 1.0f};
myModel.useLighting = true;


    while (Absolut::ScenePreview.update())
    {
        Absolut::Clear(
            0.15f,
            0.15f,
            0.15f,
            1.0f
        );




        bool moving =
            Absolut::KeyDown(VK_LEFT);



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




        Absolut::PlayerAnim.Update(
            0.015f
        );

        Absolut::SceneCamera.apply(
            1280,
            720
        );

     cube.draw();
     glDisable(GL_DEPTH_TEST);
        Absolut::PlayerAnim.Draw(
            &Absolut::PlayerImg,
            &Absolut::PlayerMap,
            Absolut::SceneCamera
        );

          if (Absolut::KeyDown('W')){

          myModel.position.z += -0.1f;

    }
     if (Absolut::KeyDown('S')){

          myModel.position.z += 0.1f;

    }
       if (Absolut::KeyDown('A')){

          myModel.position.x += -0.1f;

    }
        if (Absolut::KeyDown('D')){

       myModel.position.x += 0.1f;

    }
     if (Absolut::KeyDown(VK_SPACE)){

         myModel.position.y += 0.1f;

    }
     if (Absolut::KeyDown(VK_SHIFT)){

        myModel.position.y += -0.1f;

    }
myModel.draw();


        Absolut::SwapWindow(
            Absolut::SceneCamera
        );
           cube.rotation.x += 1;
            myModel.rotation.y += 1.0f;
    }

    Absolut::FreeAssets();

    Absolut::EndProcess();

    return 0;
}

#include "Engine/Initialize/HelloInitializer.h"
#include "GameScripts/Init.h"
#include <chrono>

int main()
{
    Absolut::HelloWorldInit();
    Absolut::InitAssets();
    Absolut::SceneCamera.mode = Absolut::ProjectionMode::Perspective;
    Absolut::SceneCamera.position = { 0.0f, 0.0f, 10.0f };
    Absolut::SceneCamera.perspNear = 0.1f;
    Absolut::SceneCamera.perspFar  = 1000.0f;
    Absolut::cube.r = 0.8f; Absolut::cube.g = 0.2f; Absolut::cube.b = 0.2f;
   if (!Absolut::myModel.LoadFromGLTF("Resources/Mesh/Kiffer_Model_GM.glb"))
{
    printf("FAILED to load model!\n");
}
else
{
    printf("Model loaded OK\n");
}
    Absolut::myModel.position = {0.0f, 0.0f, 0.0f};
    Absolut::myModel.rotation = {0.0f, 0.0f, 0.0f};
    Absolut::myModel.scale    = {1.0f, 1.0f, 1.0f};
    Absolut::myModel.useLighting = false;

    // --- start playing an animation, if the model has one ---
    if (Absolut::myModel.GetAnimationCount() > 0)
    {
        printf("Playing: %s\n", Absolut::myModel.GetAnimationName(0).c_str());
        Absolut::myModel.SetAnimation(0, true); // index 0, looping
        // or: Absolut::myModel.SetAnimation("Walk", true);
    }

    Absolut::AudioSystem.Loop("Resources/Sound/BackgroundMusic/bgm.wav");

    // --- for delta time ---
    auto lastTime = std::chrono::steady_clock::now();

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

        if (Absolut::KeyDown('W')) { Absolut::myModel.position.z += -0.1f; }
        if (Absolut::KeyDown('S')) { Absolut::myModel.position.z +=  0.1f; }
        if (Absolut::KeyDown('A')) { Absolut::myModel.position.x += -0.1f; }
        if (Absolut::KeyDown('D')) { Absolut::myModel.position.x +=  0.1f; }
        if (Absolut::KeyDown(VK_SPACE))  { Absolut::myModel.position.y +=  0.1f; }
        if (Absolut::KeyDown(VK_SHIFT))  { Absolut::myModel.position.y += -0.1f; }

        // --- advance the animation before drawing ---
        Absolut::myModel.UpdateAnimation(deltaSeconds);
        Absolut::myModel.draw();
Absolut::text.Draw("Absolut Games render test", 20.0f, 60.0f, 2.0f);
Absolut::text.SetProjection(Absolut::ScenePreview.getWidth(), Absolut::ScenePreview.getHeight());

        Absolut::SwapWindow(Absolut::SceneCamera);

        Absolut::cube.rotation.x += 1;
        Absolut::myModel.rotation.y += 1.0f;
    }

    Absolut::EndProcess();
      Absolut::text.Unload();
    return 0;
}

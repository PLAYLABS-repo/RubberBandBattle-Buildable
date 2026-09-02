#include "Player.h"

#include "../../Environment/GroundPlane.h"

namespace RubberBandBattle
{

Player::Player()
    : position(0.0f, -2.5f, 0.0f),
      collision(
          Absolut::Vec3(-0.5f, 0.0f, -0.5f),
          Absolut::Vec3(0.5f, 2.0f, 0.5f)
      ),
      verticalVelocity(0.0f),
      isGrounded(true),
      rotationY(0.0f),
      isJumping(false),
      jumpTimer(0.0f),
      Health(100),
      MaxHealth(100)
{
    UpdateCollision();
}


bool Player::IsDead() const
{
    return Health <= 0;
}


void Player::TakeDamage(int damage)
{
    if (IsDead())
        return;

    if (damage <= 0)
        return;

    Health -= damage;

    if (Health < 0)
        Health = 0;

    if (IsDead())
        OnDeath();
}


void Player::Heal(int amount)
{
    if (IsDead())
        return;

    if (amount <= 0)
        return;

    Health += amount;

    if (Health > MaxHealth)
        Health = MaxHealth;
}


void Player::SetHealth(int health)
{
    Health = health;

    if (Health < 0)
        Health = 0;

    if (Health > MaxHealth)
        Health = MaxHealth;
}


void Player::UpdateCollision()
{
    collision.SetPosition(
        Absolut::Vec3(
            position.x,
            position.y + 1.0f,
            position.z
        )
    );
}


void Player::OnDeath()
{
    isJumping = false;
    verticalVelocity = 0.0f;

    // Animation is handled by BrawlApp.cpp.
    lastAnimation = 3;
}


void Player::UpdatePlayer(float dt)
{
    if (IsDead())
        return;

    float speed = 5.0f;
    const float gravity = 20.0f;
    const float jumpForce = 8.0f;

    bool moving =
        Absolut::KeyDown('W') ||
        Absolut::KeyDown('S') ||
        Absolut::KeyDown('A') ||
        Absolut::KeyDown('D');

    // --------------------------------------------------------
    // MOVEMENT
    // --------------------------------------------------------

    if (Absolut::KeyDown(VK_SHIFT))
    {
        speed *= 2.0f;
    }

    if (Absolut::KeyDown('W'))
    {
        position.z -= speed * dt;
        rotationY = 180.0f;
    }

    if (Absolut::KeyDown('S'))
    {
        position.z += speed * dt;
        rotationY = 0.0f;
    }

    if (Absolut::KeyDown('A'))
    {
        position.x -= speed * dt;
        rotationY = -90.0f;
    }

    if (Absolut::KeyDown('D'))
    {
        position.x += speed * dt;
        rotationY = 90.0f;
    }


    if (Absolut::KeyPressed(VK_SPACE) && isGrounded)
    {
        verticalVelocity = jumpForce;
        isGrounded = false;

        isJumping = true;
        jumpTimer = 0.0f;

        lastAnimation = 1;
    }

    if (!isGrounded)
    {
        verticalVelocity -= gravity * dt;
        position.y += verticalVelocity * dt;
    }

    UpdateCollision();

    float top = GroundTop();

    if (position.y <= top &&
        verticalVelocity <= 0.0f)
    {
        position.y = top;

        verticalVelocity = 0.0f;
        isGrounded = true;

        UpdateCollision();
    }
    else
    {
        isGrounded = false;
    }

    // --------------------------------------------------------
    // JUMP TIMER
    // --------------------------------------------------------

    if (isJumping)
    {
        jumpTimer += dt;

        if (jumpTimer >= 0.5f)
            isJumping = false;
    }

    // --------------------------------------------------------
    // WALK / IDLE
    // --------------------------------------------------------

    if (!isJumping)
    {
        int animation = moving ? 4 : 0;

        if (animation != lastAnimation)
        {
            lastAnimation = animation;
        }
    }
}


#ifdef _DEBUG

std::string Player::PlayerGetAnimationName()
{
    switch (lastAnimation)
    {
        case 0:
            return "Anim_Idle";

        case 1:
            return "Anim_Jump";

        case 3:
            return "Anim_Death";

        case 4:
            return "Anim_Walk";

        default:
            return "Anim_Unknown";
    }
}


#endif

}

#pragma once

#include "Engine/dependencies/engineincludes.h"

namespace RubberBandBattle
{

class Player
{
public:

    Absolut::Vec3 position;
    Absolut::Collision collision;

    float verticalVelocity;
    bool isGrounded;

    int lastAnimation;

    bool isJumping;
    float jumpTimer;

    int Health;
    int MaxHealth;

    Player();

    bool IsDead() const;

    void TakeDamage(int damage);
    void Heal(int amount);
    void SetHealth(int health);

    void UpdateCollision();
    void OnDeath();

    void UpdatePlayer(float dt);

#ifdef _DEBUG
    std::string PlayerGetAnimationName();
#endif
};

}

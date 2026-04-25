#pragma once
#include "Enemy.h"
#include <GL/freeglut.h>
#include <cmath>

class Boss : public Enemy {
private:
    float chargeTimer;
    float chargeCooldown;
    bool isCharging;
    float chargeSpeedMultiplier;
    float bossAngle;  // for rotation animation

public:
    Boss(float x, float y, char* textureLocation, unsigned int difficulty);
    void draw() override;
    void updateCharge(float playerX, float playerY);
    bool getIsCharging() const { return isCharging; }
    float getEffectiveSpeed();
};

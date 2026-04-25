#pragma once
#include "Enemy.h"
#include <cmath>

class Boss : public Enemy {
private:
    float chargeTimer;
    float chargeCooldown;
    bool  isCharging;
    float chargeSpeedMult;
    float bossAngle;

public:
    Boss(float x, float y, char* tex, unsigned int difficulty);
    void draw() override;
    void updateCharge(float px, float py);
    bool getIsCharging() const { return isCharging; }
    float getEffectiveSpeed();
};

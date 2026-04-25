#pragma once
#include <GL/freeglut.h>
#include <cmath>

enum PerkType {
    PERK_HEALTH,
    PERK_SHIELD,
    PERK_BETTER_GUN,
    PERK_FLAMETHROWER
};

class Perk {
public:
    float x, y;
    PerkType type;
    bool active;
    int lifeTimer;     // counts down
    float pulseTimer;  // for animation

    Perk(float x, float y, PerkType type);
    void draw();
    void update();
    bool isExpired();
};

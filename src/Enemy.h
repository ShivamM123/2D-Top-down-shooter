#pragma once
#include "Entity.h"

enum ZombieType {
    ZOMBIE_WALKER = 0,   // Standard - slow, moderate HP
    ZOMBIE_RUNNER,       // Fast but fragile
    ZOMBIE_TANK,         // Huge, slow, massive HP, high damage
    ZOMBIE_WITCH,        // Screams and dashes when disturbed, high burst damage
    ZOMBIE_SPITTER,      // Ranged acid attack (spawns projectile)
    ZOMBIE_LUNGER,       // Leaps at player from far away
    ZOMBIE_TYPE_COUNT
};

class Enemy: public Entity {
public:
    ZombieType zombieType;
    float maxHealth;
    int   specialTimer;   // for abilities (witch scream, spitter spit, lunger leap)
    int   specialCooldown;
    bool  specialActive;  // is currently using ability?
    float targetX, targetY; // for lunger leap target

    Enemy(float w, float h, float speed, char* tex, float hp, float dmg, ZombieType type = ZOMBIE_WALKER);
    ~Enemy() {};
    float getMoveSpeed();
    void draw() override;
    void setDamage(float);
    void setHealth(float);
    float getHealth();
    float getDamage();
    float getMaxHealth() { return maxHealth; }
};

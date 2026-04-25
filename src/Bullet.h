#pragma once
#include <GL/freeglut.h>
#include "Entity.h"
#include <iostream>
#include <vector>

enum BulletType {
    BULLET_PISTOL,
    BULLET_RAPID,
    BULLET_FLAME
};

class Bullet: public Entity {
public:
    BulletType bulletType;

    Bullet(float x, float y, float angle, BulletType type = BULLET_PISTOL);
    ~Bullet();
    void draw() override;
    void moveBullet();
    void setDamage(float damage);
    float getDamage();
};
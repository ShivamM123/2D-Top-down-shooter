
#include "Enemy.h"
#include <math.h>

Enemy::Enemy(float width, float height, float moveSpeed, char* textureLocation, float health, float damage)
    : Entity(width, height, moveSpeed, textureLocation, health, damage) {
    sprite = new Sprite(width, height, textureLocation);
    x = 100;
    y = 100;
    origoX = width / 2;
    origoY = height / 2;
    angle = 0;
    this->health = health;
    this->damage = damage;
}

void Enemy::setDamage(float d) { damage = d; }
float Enemy::getDamage()       { return damage; }
void Enemy::setHealth(float h) { health = h; }
float Enemy::getHealth()       { return health; }
float Enemy::getMoveSpeed()    { return moveSpeed; }

void Enemy::draw() {
    glMatrixMode(GL_MODELVIEW);  // ensure correct mode BEFORE identity load
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(x + origoX, y + origoY, 0);
    glRotatef(angle, 0, 0, 1);
    sprite->draw();
    glPopMatrix();
}
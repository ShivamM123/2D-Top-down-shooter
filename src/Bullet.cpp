#include "Bullet.h"
#include <cmath>

Bullet::Bullet(float startX, float startY, float ang, BulletType type)
    : Entity(14, 14, (type == BULLET_FLAME ? 8.0f : (type == BULLET_RAPID ? 25.0f : 18.0f)),
             nullptr, 1,
             (type == BULLET_FLAME ? 15.0f : (type == BULLET_RAPID ? 12.0f : 20.0f))),
      bulletType(type)
{
    x = startX;
    y = startY;
    angle = ang;
    origoX = 7;
    origoY = 7;
    sprite = nullptr;
}

Bullet::~Bullet() {}

float Bullet::getDamage() { return damage; }
void Bullet::setDamage(float d) { damage = d; }

void Bullet::moveBullet() {
    x += moveSpeed * cosf(angle);
    y += moveSpeed * sinf(angle);
}

void Bullet::draw() {
    float cx = x;
    float cy = y;

    glPushMatrix();
    glTranslatef(cx, cy, 0);
    glRotatef(angle * 180.0f / 3.14159f, 0, 0, 1);

    switch (bulletType) {
    case BULLET_PISTOL: {
        // Elongated yellow oval with white glowing core
        // Trail (faded)
        glColor4f(1.0f, 0.8f, 0.0f, 0.3f);
        glBegin(GL_QUADS);
        glVertex2f(-14, -3); glVertex2f(2, -3);
        glVertex2f(2, 3);   glVertex2f(-14, 3);
        glEnd();
        // Body
        glColor3f(1.0f, 0.85f, 0.1f);
        glBegin(GL_POLYGON);
        for (int i = 0; i <= 16; i++) {
            float a = i * 3.14159f / 8.0f;
            glVertex2f(6 * cosf(a), 3 * sinf(a));
        }
        glEnd();
        // Core glow
        glColor3f(1.0f, 1.0f, 0.8f);
        glBegin(GL_POLYGON);
        for (int i = 0; i <= 16; i++) {
            float a = i * 3.14159f / 8.0f;
            glVertex2f(3 * cosf(a), 1.5f * sinf(a));
        }
        glEnd();
        break;
    }
    case BULLET_RAPID: {
        // Cyan glowing circle
        // Outer glow
        glColor4f(0.0f, 0.9f, 1.0f, 0.35f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 16; i++) {
            float a = i * 6.2832f / 16;
            glVertex2f(9 * cosf(a), 9 * sinf(a));
        }
        glEnd();
        // Main circle
        glColor3f(0.1f, 0.9f, 1.0f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 16; i++) {
            float a = i * 6.2832f / 16;
            glVertex2f(6 * cosf(a), 6 * sinf(a));
        }
        glEnd();
        // White core
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 16; i++) {
            float a = i * 6.2832f / 16;
            glVertex2f(2.5f * cosf(a), 2.5f * sinf(a));
        }
        glEnd();
        break;
    }
    case BULLET_FLAME: {
        // Orange teardrop / flame shape
        // Outer flame
        glColor4f(1.0f, 0.35f, 0.0f, 0.7f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(8, 0);
        for (int i = 0; i <= 12; i++) {
            float a = -1.0f + i * 2.0f / 12.0f;
            float r = 5.0f * (1.0f - a * a * 0.5f);
            glVertex2f(a * 4.0f, r * sinf(a * 1.5708f + 1.5708f));
        }
        glEnd();
        glBegin(GL_POLYGON);
        for (int i = 0; i < 12; i++) {
            float a = i * 6.2832f / 12;
            glVertex2f(6 * cosf(a), 4 * sinf(a));
        }
        glEnd();
        // Core
        glColor3f(1.0f, 0.8f, 0.0f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 10; i++) {
            float a = i * 6.2832f / 10;
            glVertex2f(3 * cosf(a), 2 * sinf(a));
        }
        glEnd();
        break;
    }
    }

    glPopMatrix();
}

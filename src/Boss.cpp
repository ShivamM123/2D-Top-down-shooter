#include "Boss.h"
#include <cmath>

Boss::Boss(float x, float y, char* textureLocation, unsigned int difficulty)
    : Enemy(120, 120, 0.6f, textureLocation, 500.0f, 30.0f),
      chargeTimer(0), chargeCooldown(300), isCharging(false),
      chargeSpeedMultiplier(4.0f), bossAngle(0.0f)
{
    this->x = x;
    this->y = y;
    // Scale with difficulty
    if (difficulty == 0) { health = 350; damage = 20; }
    else if (difficulty == 2) { health = 700; damage = 45; chargeSpeedMultiplier = 6.0f; }
}

void Boss::updateCharge(float playerX, float playerY) {
    bossAngle += 1.0f;
    if (bossAngle > 360) bossAngle -= 360;

    chargeTimer++;
    if (chargeTimer >= chargeCooldown) {
        isCharging = true;
        chargeTimer = 0;
    }
    // Charge lasts 30 ticks
    if (isCharging && chargeTimer > 30) {
        isCharging = false;
    }
}

float Boss::getEffectiveSpeed() {
    return isCharging ? moveSpeed * chargeSpeedMultiplier : moveSpeed;
}

static void drawNGonBoss(float cx, float cy, float r, int n, float rotDeg = 0) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < n; i++) {
        float a = rotDeg * 3.14159f / 180.0f + i * 6.2832f / n;
        glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
    }
    glEnd();
}

static void drawNGonOutlineBoss(float cx, float cy, float r, int n, float rotDeg = 0) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < n; i++) {
        float a = rotDeg * 3.14159f / 180.0f + i * 6.2832f / n;
        glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
    }
    glEnd();
}

void Boss::draw() {
    float cx = x + width / 2.0f;
    float cy = y + height / 2.0f;
    float r = width / 2.0f;
    float hpRatio = health / (health <= 500 ? 500.0f : 700.0f);

    glLineWidth(3.0f);

    // Outer pulsing aura
    float pulseR = r + 8 + 5 * sinf(bossAngle * 0.05f);
    glColor4f(0.8f, 0.0f, 0.1f, 0.4f);
    drawNGonBoss(cx, cy, pulseR, 32);

    // Body — rotating octagon
    if (isCharging)
        glColor3f(1.0f, 0.3f, 0.0f); // orange when charging
    else
        glColor3f(0.7f, 0.05f, 0.05f); // dark red normal
    drawNGonBoss(cx, cy, r, 8, bossAngle);

    // Inner details — spikes
    glColor3f(0.4f, 0.0f, 0.0f);
    drawNGonBoss(cx, cy, r * 0.7f, 8, bossAngle + 22.5f);

    // Eyes
    float eyeOff = r * 0.3f;
    // Left eye
    glColor3f(1.0f, 0.8f, 0.0f);
    drawNGonBoss(cx - eyeOff, cy - eyeOff * 0.4f, r * 0.18f, 32);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawNGonBoss(cx - eyeOff, cy - eyeOff * 0.4f, r * 0.09f, 32);
    // Right eye
    glColor3f(1.0f, 0.8f, 0.0f);
    drawNGonBoss(cx + eyeOff, cy - eyeOff * 0.4f, r * 0.18f, 32);
    glColor3f(0.0f, 0.0f, 0.0f);
    drawNGonBoss(cx + eyeOff, cy - eyeOff * 0.4f, r * 0.09f, 32);

    // Mouth — jagged line
    glColor3f(1.0f, 0.9f, 0.0f);
    glLineWidth(2.5f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 8; i++) {
        float mx = cx - r * 0.45f + i * (r * 0.9f / 8.0f);
        float my = cy + r * 0.35f + (i % 2 == 0 ? r * 0.1f : -r * 0.1f);
        glVertex2f(mx, my);
    }
    glEnd();

    // HP-colored outline
    glColor3f(1.0f - hpRatio, hpRatio, 0.0f);
    drawNGonOutlineBoss(cx, cy, r + 2, 8, bossAngle);

    // BOSS label
    glColor3f(1.0f, 0.2f, 0.2f);
    glRasterPos2f(cx - 15, cy - r - 14);
    const char* label = "BOSS";
    for (const char* c = label; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glLineWidth(1.0f);
}

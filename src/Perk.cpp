#include "Perk.h"

Perk::Perk(float x, float y, PerkType type)
    : x(x), y(y), type(type), active(true), lifeTimer(1000), pulseTimer(0.0f) {}

void Perk::update() {
    lifeTimer--;
    pulseTimer += 0.1f;
    if (pulseTimer > 6.28f) pulseTimer -= 6.28f;
}

bool Perk::isExpired() {
    return lifeTimer <= 0;
}

static void drawNGon(float cx, float cy, float r, int n) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < n; i++) {
        float a = i * 6.2832f / n;
        glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
    }
    glEnd();
}

static void drawNGonOutline(float cx, float cy, float r, int n) {
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < n; i++) {
        float a = i * 6.2832f / n;
        glVertex2f(cx + r * cosf(a), cy + r * sinf(a));
    }
    glEnd();
}

void Perk::draw() {
    if (!active) return;

    float pulse = 0.85f + 0.15f * sinf(pulseTimer);
    float r = 18.0f * pulse;
    float cx = x + 35.0f; // center on entity-sized block
    float cy = y + 35.0f;

    glLineWidth(2.0f);

    // Draw dark backing circle
    glColor3f(0.05f, 0.05f, 0.08f);
    drawNGon(cx, cy, r + 4, 32);

    switch (type) {
    case PERK_HEALTH:
        // Green cross
        glColor3f(0.1f, 0.9f, 0.2f);
        glBegin(GL_QUADS);
        glVertex2f(cx - r*0.3f, cy - r); glVertex2f(cx + r*0.3f, cy - r);
        glVertex2f(cx + r*0.3f, cy + r); glVertex2f(cx - r*0.3f, cy + r);
        glEnd();
        glBegin(GL_QUADS);
        glVertex2f(cx - r, cy - r*0.3f); glVertex2f(cx + r, cy - r*0.3f);
        glVertex2f(cx + r, cy + r*0.3f); glVertex2f(cx - r, cy + r*0.3f);
        glEnd();
        glColor3f(0.5f, 1.0f, 0.5f);
        drawNGonOutline(cx, cy, r + 2, 32);
        break;

    case PERK_SHIELD:
        // Blue hexagon
        glColor3f(0.2f, 0.5f, 1.0f);
        drawNGon(cx, cy, r, 6);
        glColor3f(0.6f, 0.8f, 1.0f);
        drawNGon(cx, cy, r * 0.6f, 6);
        glColor3f(1.0f, 1.0f, 1.0f);
        drawNGonOutline(cx, cy, r, 6);
        break;

    case PERK_BETTER_GUN:
        // Yellow star
        glColor3f(1.0f, 0.85f, 0.0f);
        glBegin(GL_POLYGON);
        for (int i = 0; i < 10; i++) {
            float a = i * 6.2832f / 10.0f - 1.5708f;
            float rad = (i % 2 == 0) ? r : r * 0.45f;
            glVertex2f(cx + rad * cosf(a), cy + rad * sinf(a));
        }
        glEnd();
        glColor3f(1.0f, 1.0f, 0.4f);
        drawNGonOutline(cx, cy, r + 2, 32);
        break;

    case PERK_FLAMETHROWER:
        // Orange/red flame shape (triangle fan)
        glColor3f(1.0f, 0.4f, 0.0f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int i = 0; i <= 16; i++) {
            float a = (i / 16.0f) * 6.2832f;
            float rad = (i % 2 == 0) ? r : r * 0.7f;
            glVertex2f(cx + rad * cosf(a), cy + rad * sinf(a));
        }
        glEnd();
        glColor3f(1.0f, 0.8f, 0.2f);
        drawNGon(cx, cy, r * 0.35f, 32); // inner core
        glColor3f(1.0f, 0.5f, 0.0f);
        drawNGonOutline(cx, cy, r, 20);
        break;
    }

    // Label
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(cx - 15, cy + r + 12);
    const char* label = "";
    switch(type) {
    case PERK_HEALTH:       label = "HEALTH"; break;
    case PERK_SHIELD:       label = "SHIELD"; break;
    case PERK_BETTER_GUN:   label = "RAPID"; break;
    case PERK_FLAMETHROWER: label = "FLAME"; break;
    }
    for (const char* c = label; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *c);

    glLineWidth(1.0f);
}

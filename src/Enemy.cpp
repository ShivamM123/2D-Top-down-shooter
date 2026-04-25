#include "Enemy.h"
#include <cmath>

static void drawCircle(float cx, float cy, float r, int segs) {
    glBegin(GL_POLYGON);
    for(int i=0;i<segs;i++){float a=i*6.2832f/segs; glVertex2f(cx+r*cosf(a),cy+r*sinf(a));}
    glEnd();
}
static void drawCircleOutline(float cx, float cy, float r, int segs) {
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<segs;i++){float a=i*6.2832f/segs; glVertex2f(cx+r*cosf(a),cy+r*sinf(a));}
    glEnd();
}

Enemy::Enemy(float w, float h, float speed, char* tex, float hp, float dmg, ZombieType type)
    : Entity(w, h, speed, nullptr, hp, dmg),
      zombieType(type), maxHealth(hp),
      specialTimer(0), specialCooldown(0),
      specialActive(false), targetX(0), targetY(0)
{
    sprite = nullptr; // All zombies drawn geometrically now
    x = 100; y = 100;
    origoX = w/2; origoY = h/2;
    this->health = hp;
    this->damage = dmg;

    // Set special cooldowns per type
    switch(type) {
    case ZOMBIE_WITCH:   specialCooldown = 200; break;
    case ZOMBIE_SPITTER: specialCooldown = 150; break;
    case ZOMBIE_LUNGER:  specialCooldown = 180; break;
    default: specialCooldown = 0; break;
    }
}

void Enemy::setDamage(float d)  { damage = d; }
float Enemy::getDamage()        { return damage; }
void Enemy::setHealth(float h)  { health = h; }
float Enemy::getHealth()        { return health; }
float Enemy::getMoveSpeed()     { return specialActive ? moveSpeed*3.0f : moveSpeed; }

void Enemy::draw() {
    float cx = x + width/2;
    float cy = y + height/2;
    float r  = width/2;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    switch(zombieType) {
    case ZOMBIE_WALKER: {
        // Green-grey shambling zombie
        glColor3f(0.30f, 0.45f, 0.25f);
        drawCircle(cx, cy, r*0.85f, 20);
        // Dark inner
        glColor3f(0.20f, 0.30f, 0.15f);
        drawCircle(cx, cy, r*0.55f, 16);
        // Eyes - red dots
        float ea = angle*3.14159f/180.0f;
        float eyeOff = r*0.3f;
        glColor3f(0.9f, 0.15f, 0.1f);
        drawCircle(cx+cosf(ea)*eyeOff - sinf(ea)*5, cy+sinf(ea)*eyeOff + cosf(ea)*5, 3, 8);
        drawCircle(cx+cosf(ea)*eyeOff + sinf(ea)*5, cy+sinf(ea)*eyeOff - cosf(ea)*5, 3, 8);
        // Outline
        glColor3f(0.15f, 0.20f, 0.10f);
        drawCircleOutline(cx, cy, r*0.85f, 20);
        break;
    }
    case ZOMBIE_RUNNER: {
        // Lean red-brown, smaller, jagged outline
        glColor3f(0.55f, 0.25f, 0.15f);
        // Elongated body (drawn as 2 overlapping circles)
        float ea2 = angle*3.14159f/180.0f;
        drawCircle(cx - cosf(ea2)*r*0.15f, cy - sinf(ea2)*r*0.15f, r*0.65f, 16);
        drawCircle(cx + cosf(ea2)*r*0.25f, cy + sinf(ea2)*r*0.25f, r*0.55f, 16);
        // Eyes - yellow, frantic
        glColor3f(1.0f, 0.9f, 0.1f);
        drawCircle(cx+cosf(ea2)*r*0.4f - sinf(ea2)*4, cy+sinf(ea2)*r*0.4f + cosf(ea2)*4, 2.5f, 8);
        drawCircle(cx+cosf(ea2)*r*0.4f + sinf(ea2)*4, cy+sinf(ea2)*r*0.4f - cosf(ea2)*4, 2.5f, 8);
        // Speed lines
        glColor3f(0.8f, 0.4f, 0.2f);
        glLineWidth(1.5f);
        glBegin(GL_LINES);
        for (int i=0;i<3;i++) {
            float off = (i-1)*7.0f;
            glVertex2f(cx-cosf(ea2)*r*0.9f+sinf(ea2)*off, cy-sinf(ea2)*r*0.9f-cosf(ea2)*off);
            glVertex2f(cx-cosf(ea2)*r*1.4f+sinf(ea2)*off, cy-sinf(ea2)*r*1.4f-cosf(ea2)*off);
        }
        glEnd();
        glLineWidth(1.0f);
        break;
    }
    case ZOMBIE_TANK: {
        // Massive dark grey/purple body
        glColor3f(0.35f, 0.20f, 0.35f);
        drawCircle(cx, cy, r, 24);
        // Armor plates
        glColor3f(0.25f, 0.12f, 0.25f);
        drawCircle(cx, cy, r*0.75f, 8); // octagon inner
        // Shoulder bumps
        float ea3 = angle*3.14159f/180.0f;
        glColor3f(0.40f, 0.22f, 0.40f);
        drawCircle(cx-sinf(ea3)*r*0.6f, cy+cosf(ea3)*r*0.6f, r*0.3f, 12);
        drawCircle(cx+sinf(ea3)*r*0.6f, cy-cosf(ea3)*r*0.6f, r*0.3f, 12);
        // Eyes - small angry red
        glColor3f(1.0f, 0.0f, 0.0f);
        drawCircle(cx+cosf(ea3)*r*0.35f-sinf(ea3)*6, cy+sinf(ea3)*r*0.35f+cosf(ea3)*6, 4, 8);
        drawCircle(cx+cosf(ea3)*r*0.35f+sinf(ea3)*6, cy+sinf(ea3)*r*0.35f-cosf(ea3)*6, 4, 8);
        // Heavy outline
        glColor3f(0.15f, 0.05f, 0.15f);
        glLineWidth(2.5f);
        drawCircleOutline(cx, cy, r, 24);
        glLineWidth(1.0f);
        break;
    }
    case ZOMBIE_WITCH: {
        // Pale white/blue, long hair streaks
        glColor3f(0.75f, 0.78f, 0.85f);
        drawCircle(cx, cy, r*0.7f, 20);
        // Hair streaks (lines radiating outward)
        float ea4 = angle*3.14159f/180.0f;
        glColor3f(0.50f, 0.50f, 0.55f);
        glLineWidth(2.0f);
        glBegin(GL_LINES);
        for (int i=0;i<6;i++) {
            float ha = ea4 + 3.14159f + (i-2.5f)*0.4f; // behind head
            glVertex2f(cx+cosf(ha)*r*0.5f, cy+sinf(ha)*r*0.5f);
            glVertex2f(cx+cosf(ha)*r*1.3f, cy+sinf(ha)*r*1.3f);
        }
        glEnd();
        glLineWidth(1.0f);
        // Eyes - glowing when special active
        if (specialActive) {
            glColor3f(1.0f, 0.0f, 0.0f);
            drawCircle(cx+cosf(ea4)*r*0.3f-sinf(ea4)*4, cy+sinf(ea4)*r*0.3f+cosf(ea4)*4, 4, 8);
            drawCircle(cx+cosf(ea4)*r*0.3f+sinf(ea4)*4, cy+sinf(ea4)*r*0.3f-cosf(ea4)*4, 4, 8);
        } else {
            glColor3f(0.4f, 0.6f, 0.8f);
            drawCircle(cx+cosf(ea4)*r*0.3f-sinf(ea4)*4, cy+sinf(ea4)*r*0.3f+cosf(ea4)*4, 3, 8);
            drawCircle(cx+cosf(ea4)*r*0.3f+sinf(ea4)*4, cy+sinf(ea4)*r*0.3f-cosf(ea4)*4, 3, 8);
        }
        // Mouth scream when active
        if (specialActive) {
            glColor3f(0.1f, 0.0f, 0.0f);
            drawCircle(cx+cosf(ea4)*r*0.15f, cy+sinf(ea4)*r*0.15f, 5, 8);
        }
        break;
    }
    case ZOMBIE_SPITTER: {
        // Green/yellow bloated body
        glColor3f(0.45f, 0.60f, 0.15f);
        drawCircle(cx, cy, r*0.85f, 20);
        // Bloated belly
        glColor3f(0.55f, 0.70f, 0.10f);
        drawCircle(cx, cy, r*0.50f, 16);
        // Acid drool
        float ea5 = angle*3.14159f/180.0f;
        glColor3f(0.3f, 0.8f, 0.1f);
        glLineWidth(2.0f);
        float mx = cx+cosf(ea5)*r*0.5f, my = cy+sinf(ea5)*r*0.5f;
        glBegin(GL_LINES);
        glVertex2f(mx, my); glVertex2f(mx+cosf(ea5)*12, my+sinf(ea5)*12+5);
        glVertex2f(mx, my); glVertex2f(mx+cosf(ea5)*8,  my+sinf(ea5)*8+8);
        glEnd();
        glLineWidth(1.0f);
        // Eyes
        glColor3f(0.9f, 0.9f, 0.0f);
        drawCircle(cx+cosf(ea5)*r*0.3f-sinf(ea5)*5, cy+sinf(ea5)*r*0.3f+cosf(ea5)*5, 3, 8);
        drawCircle(cx+cosf(ea5)*r*0.3f+sinf(ea5)*5, cy+sinf(ea5)*r*0.3f-cosf(ea5)*5, 3, 8);
        break;
    }
    case ZOMBIE_LUNGER: {
        // Dark blue-grey, crouched shape
        float ea6 = angle*3.14159f/180.0f;
        glColor3f(0.25f, 0.28f, 0.40f);
        // Crouched oval
        drawCircle(cx, cy, r*0.6f, 16);
        // Long arms reaching forward
        glColor3f(0.35f, 0.35f, 0.50f);
        glLineWidth(3.0f);
        glBegin(GL_LINES);
        float armLen = specialActive ? r*1.8f : r*1.1f;
        glVertex2f(cx-sinf(ea6)*r*0.3f, cy+cosf(ea6)*r*0.3f);
        glVertex2f(cx-sinf(ea6)*r*0.3f+cosf(ea6)*armLen, cy+cosf(ea6)*r*0.3f+sinf(ea6)*armLen);
        glVertex2f(cx+sinf(ea6)*r*0.3f, cy-cosf(ea6)*r*0.3f);
        glVertex2f(cx+sinf(ea6)*r*0.3f+cosf(ea6)*armLen, cy-cosf(ea6)*r*0.3f+sinf(ea6)*armLen);
        glEnd();
        glLineWidth(1.0f);
        // Claws
        glColor3f(0.8f, 0.2f, 0.1f);
        float clawX = cx+cosf(ea6)*armLen;
        float clawY = cy+sinf(ea6)*armLen;
        drawCircle(clawX-sinf(ea6)*r*0.3f, clawY+cosf(ea6)*r*0.3f, 4, 6);
        drawCircle(clawX+sinf(ea6)*r*0.3f, clawY-cosf(ea6)*r*0.3f, 4, 6);
        // Eyes
        glColor3f(0.9f, 0.5f, 0.0f);
        drawCircle(cx+cosf(ea6)*r*0.2f-sinf(ea6)*4, cy+sinf(ea6)*r*0.2f+cosf(ea6)*4, 3, 8);
        drawCircle(cx+cosf(ea6)*r*0.2f+sinf(ea6)*4, cy+sinf(ea6)*r*0.2f-cosf(ea6)*4, 3, 8);
        break;
    }
    default: break;
    }

    glPopMatrix();
}
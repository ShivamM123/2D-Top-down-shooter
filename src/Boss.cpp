#include "Boss.h"
#include <cmath>

static void bossCircle(float cx, float cy, float r, int n) {
    glBegin(GL_POLYGON);
    for(int i=0;i<n;i++){float a=i*6.2832f/n; glVertex2f(cx+r*cosf(a),cy+r*sinf(a));}
    glEnd();
}
static void bossCircleOutline(float cx, float cy, float r, int n) {
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<n;i++){float a=i*6.2832f/n; glVertex2f(cx+r*cosf(a),cy+r*sinf(a));}
    glEnd();
}

Boss::Boss(float bx, float by, char* tex, unsigned int difficulty)
    : Enemy(130, 130, 0.7f, tex, 800.0f, 40.0f, ZOMBIE_TANK),
      chargeTimer(0), chargeCooldown(250), isCharging(false),
      chargeSpeedMult(4.5f), bossAngle(0)
{
    x = bx; y = by;
    if (difficulty==0){ health=500; damage=25; chargeCooldown=350; }
    if (difficulty==2){ health=1200; damage=60; chargeSpeedMult=6.0f; chargeCooldown=150; }
    maxHealth = health;
}

void Boss::updateCharge(float px, float py) {
    bossAngle += 1.2f; if(bossAngle>360) bossAngle-=360;
    chargeTimer++;
    if (chargeTimer>=chargeCooldown) { isCharging=true; chargeTimer=0; }
    if (isCharging && chargeTimer>35) isCharging=false;
}

float Boss::getEffectiveSpeed() {
    return isCharging ? moveSpeed*chargeSpeedMult : moveSpeed;
}

void Boss::draw() {
    float cx = x+width/2, cy = y+height/2, r = width/2;
    float hpR = health/maxHealth;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glLineWidth(3.0f);

    // Pulsing aura
    float pulseR = r + 10 + 6*sinf(bossAngle*0.06f);
    glColor3f(0.5f, 0.05f, 0.08f);
    bossCircle(cx, cy, pulseR, 32);

    // Body — rotating octagon
    if (isCharging) glColor3f(1.0f, 0.3f, 0.0f);
    else glColor3f(0.6f, 0.04f, 0.04f);
    // Rotating octagon
    glBegin(GL_POLYGON);
    for(int i=0;i<8;i++){
        float a=bossAngle*3.14159f/180.0f + i*6.2832f/8;
        glVertex2f(cx+r*cosf(a), cy+r*sinf(a));
    }
    glEnd();

    // Inner spikes
    glColor3f(0.35f, 0.0f, 0.0f);
    glBegin(GL_POLYGON);
    for(int i=0;i<8;i++){
        float a=(bossAngle+22.5f)*3.14159f/180.0f + i*6.2832f/8;
        glVertex2f(cx+r*0.7f*cosf(a), cy+r*0.7f*sinf(a));
    }
    glEnd();

    // Shoulder armor
    float fa = angle*3.14159f/180.0f;
    glColor3f(0.45f, 0.08f, 0.08f);
    bossCircle(cx-sinf(fa)*r*0.6f, cy+cosf(fa)*r*0.6f, r*0.35f, 12);
    bossCircle(cx+sinf(fa)*r*0.6f, cy-cosf(fa)*r*0.6f, r*0.35f, 12);

    // Eyes
    float eyeOff=r*0.28f;
    glColor3f(1.0f, 0.8f, 0.0f);
    bossCircle(cx+cosf(fa)*eyeOff-sinf(fa)*8, cy+sinf(fa)*eyeOff+cosf(fa)*8, r*0.15f, 16);
    bossCircle(cx+cosf(fa)*eyeOff+sinf(fa)*8, cy+sinf(fa)*eyeOff-cosf(fa)*8, r*0.15f, 16);
    glColor3f(0.0f, 0.0f, 0.0f);
    bossCircle(cx+cosf(fa)*eyeOff-sinf(fa)*8, cy+sinf(fa)*eyeOff+cosf(fa)*8, r*0.07f, 12);
    bossCircle(cx+cosf(fa)*eyeOff+sinf(fa)*8, cy+sinf(fa)*eyeOff-cosf(fa)*8, r*0.07f, 12);

    // Mouth — jagged
    glColor3f(1.0f, 0.85f, 0.0f);
    glLineWidth(2.5f);
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<=8;i++){
        float off=(i-4)*r*0.09f;
        float fwd=r*0.40f+(i%2==0?r*0.08f:-r*0.08f);
        glVertex2f(cx+cosf(fa)*fwd-sinf(fa)*off, cy+sinf(fa)*fwd+cosf(fa)*off);
    }
    glEnd();

    // HP-tinted outline
    glColor3f(1.0f-hpR, hpR, 0.0f);
    glLineWidth(3.0f);
    bossCircleOutline(cx, cy, r+3, 8);

    // Label
    glColor3f(1.0f, 0.15f, 0.15f);
    glRasterPos2f(cx-18, cy-r-16);
    const char* lbl = isCharging ? "!! BOSS !!" : "BOSS";
    for(const char* c=lbl;*c;c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12,(int)*c);

    glLineWidth(1.0f);
    glPopMatrix();
}

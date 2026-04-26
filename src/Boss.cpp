#include "Boss.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>

// ─── Helper drawing functions ───
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

// ═══════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════
Boss::Boss(float bx, float by, char* tex, unsigned int difficulty)
    : Enemy(130, 130, 0.7f, tex, 2500.0f, 40.0f, ZOMBIE_TANK),
      currentPhase(BOSS_PHASE_CHASE),
      currentAction(BOSS_ACTION_CHASE),
      actionTimer(0),
      throwCooldownMax(500),
      throwCooldownTimer(0),
      throwWindupFrames(60),
      hasRoared(false),
      enrageSpeedMult(3.5f),
      chargeDirectionX(0), chargeDirectionY(0),
      chargeDuration(80),
      chargeTimer(0),
      chargeCooldownTimer(0),
      chargeCooldownMax(200),
      hasSummonedSwarm(false),
      swarmCount(15),
      bossAngle(0),
      roarScale(1.0f),
      hitFlashTimer(0),
      shakeIntensity(0)
{
    x = bx; y = by;
    if (difficulty == 0) {
        health = 1500; damage = 25;
        throwCooldownMax = 600;
        chargeCooldownMax = 300;
    }
    if (difficulty == 2) {
        health = 4000; damage = 60;
        enrageSpeedMult = 4.5f;
        throwCooldownMax = 350;
        chargeCooldownMax = 140;
    }
    maxHealth = health;
}

// ═══════════════════════════════════════════════════════════════
// Phase Transition Logic
// ═══════════════════════════════════════════════════════════════
void Boss::checkPhaseTransition() {
    float hpRatio = health / maxHealth;

    if (currentPhase == BOSS_PHASE_CHASE && hpRatio <= 0.50f) {
        // Transition to ENRAGE
        currentPhase = BOSS_PHASE_ENRAGE;
        currentAction = BOSS_ACTION_ROAR;
        actionTimer = 0;
        hasRoared = false;
        roarScale = 1.0f;
        shakeIntensity = 12.0f;  // Big shake on phase transition
    }
    else if (currentPhase == BOSS_PHASE_ENRAGE && hpRatio <= 0.15f) {
        // Transition to DESPERATION
        currentPhase = BOSS_PHASE_DESPERATION;
        currentAction = BOSS_ACTION_SUMMON;
        actionTimer = 0;
        hasSummonedSwarm = false;
        shakeIntensity = 18.0f;  // Massive shake
    }
}

// ═══════════════════════════════════════════════════════════════
// Core Update — called every frame
// ═══════════════════════════════════════════════════════════════
void Boss::update(float playerX, float playerY, int screenW, int screenH) {
    bossAngle += 1.2f;
    if (bossAngle > 360) bossAngle -= 360;

    if (hitFlashTimer > 0) hitFlashTimer--;
    if (shakeIntensity > 0) shakeIntensity *= 0.92f; // Decay shake
    if (shakeIntensity < 0.1f) shakeIntensity = 0;

    checkPhaseTransition();
    actionTimer++;

    switch (currentPhase) {
    // ─────────────────────────────────────────────────
    // PHASE 1: THE CHASE
    // ─────────────────────────────────────────────────
    case BOSS_PHASE_CHASE: {
        throwCooldownTimer++;

        if (currentAction == BOSS_ACTION_THROW) {
            // Winding up to throw
            if (actionTimer >= (int)throwWindupFrames) {
                throwProjectile(playerX, playerY);
                currentAction = BOSS_ACTION_CHASE;
                actionTimer = 0;
                throwCooldownTimer = 0;
            }
        }
        else {
            // Standard chase, periodically throw
            if (throwCooldownTimer >= throwCooldownMax) {
                currentAction = BOSS_ACTION_THROW;
                actionTimer = 0;
            }
        }
        // Subtle screen shake while walking (the "Tank" footsteps)
        if (actionTimer % 40 == 0) {
            shakeIntensity = 2.0f;
        }
        break;
    }

    // ─────────────────────────────────────────────────
    // PHASE 2: ENRAGE
    // ─────────────────────────────────────────────────
    case BOSS_PHASE_ENRAGE: {
        if (currentAction == BOSS_ACTION_ROAR) {
            // Roar animation: lasts ~120 frames, boss is stationary
            roarScale = 1.0f + 0.3f * sinf(actionTimer * 0.15f);
            shakeIntensity = 6.0f + 4.0f * sinf(actionTimer * 0.1f);

            if (actionTimer >= 120) {
                hasRoared = true;
                currentAction = BOSS_ACTION_CHASE;
                actionTimer = 0;
                chargeCooldownTimer = 0;
                roarScale = 1.0f;
            }
        }
        else if (currentAction == BOSS_ACTION_CHARGE) {
            chargeTimer++;
            // Charge in a straight line
            x += chargeDirectionX * moveSpeed * enrageSpeedMult;
            y += chargeDirectionY * moveSpeed * enrageSpeedMult;

            // Did we hit a wall?
            bool hitWall = false;
            if (x < 0) { x = 0; hitWall = true; }
            if (y < 0) { y = 0; hitWall = true; }
            if (x + width > screenW) { x = (float)(screenW - width); hitWall = true; }
            if (y + height > screenH) { y = (float)(screenH - height); hitWall = true; }

            if (hitWall) {
                shakeIntensity = 15.0f; // VIOLENT screen shake on wall impact
                currentAction = BOSS_ACTION_STUN;
                actionTimer = 0;
            }
            else if (chargeTimer >= chargeDuration) {
                currentAction = BOSS_ACTION_CHASE;
                actionTimer = 0;
                chargeCooldownTimer = 0;
            }
        }
        else if (currentAction == BOSS_ACTION_STUN) {
            // Stunned for 60 frames after hitting a wall
            if (actionTimer >= 60) {
                currentAction = BOSS_ACTION_CHASE;
                actionTimer = 0;
                chargeCooldownTimer = 0;
            }
        }
        else {
            // Chase, but periodically initiate a charge
            chargeCooldownTimer++;
            if (chargeCooldownTimer >= chargeCooldownMax) {
                // Lock charge direction toward player
                float dx = playerX - (x + width/2);
                float dy = playerY - (y + height/2);
                float len = sqrtf(dx*dx + dy*dy);
                if (len > 0.1f) { chargeDirectionX = dx/len; chargeDirectionY = dy/len; }
                else { chargeDirectionX = 1; chargeDirectionY = 0; }

                currentAction = BOSS_ACTION_CHARGE;
                chargeTimer = 0;
                actionTimer = 0;
                shakeIntensity = 5.0f;
            }

            // Still footstep shake
            if (actionTimer % 25 == 0) {
                shakeIntensity = 3.0f;
            }
        }
        break;
    }

    // ─────────────────────────────────────────────────
    // PHASE 3: DESPERATION
    // ─────────────────────────────────────────────────
    case BOSS_PHASE_DESPERATION: {
        if (currentAction == BOSS_ACTION_SUMMON) {
            // Summon animation — boss pauses, then Game reads needsSwarmSpawn()
            if (actionTimer >= 90) {
                currentAction = BOSS_ACTION_CHASE;
                actionTimer = 0;
            }
            shakeIntensity = 4.0f;
        }
        else {
            // Desperation chase is faster than Phase 1 but no charges
            // Heavy footsteps
            if (actionTimer % 20 == 0) {
                shakeIntensity = 3.5f;
            }
        }
        break;
    }
    }

    // Update projectiles
    updateProjectiles();
}

// ═══════════════════════════════════════════════════════════════
// Projectile Throw
// ═══════════════════════════════════════════════════════════════
void Boss::throwProjectile(float playerX, float playerY) {
    BossProjectile p;
    p.x = x + width/2;
    p.y = y + height/2;
    float dx = playerX - p.x;
    float dy = playerY - p.y;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 1.0f) len = 1.0f;
    float speed = 6.0f;
    p.vx = (dx/len) * speed;
    p.vy = (dy/len) * speed;
    p.radius = 18;
    p.damage = 35;
    p.lifetime = 180;
    p.active = true;
    projectiles.push_back(p);
    shakeIntensity = 4.0f; // Shake on throw
}

void Boss::updateProjectiles() {
    for (auto it = projectiles.begin(); it != projectiles.end(); ) {
        if (!it->active) { it = projectiles.erase(it); continue; }
        it->x += it->vx;
        it->y += it->vy;
        it->lifetime--;
        if (it->lifetime <= 0) {
            it->active = false;
        }
        ++it;
    }
}

// ═══════════════════════════════════════════════════════════════
// Movement — called from Game::moveEnemy
// ═══════════════════════════════════════════════════════════════
float Boss::getEffectiveSpeed() {
    switch (currentAction) {
    case BOSS_ACTION_ROAR:
    case BOSS_ACTION_THROW:
    case BOSS_ACTION_SUMMON:
    case BOSS_ACTION_STUN:
        return 0;  // Stationary during these actions
    case BOSS_ACTION_CHARGE:
        return 0;  // Charge movement is handled internally in update()
    default:
        break;
    }

    switch (currentPhase) {
    case BOSS_PHASE_CHASE:       return moveSpeed;
    case BOSS_PHASE_ENRAGE:      return moveSpeed * 1.5f;   // Faster between charges
    case BOSS_PHASE_DESPERATION: return moveSpeed * 2.0f;   // Desperate speed
    default:                     return moveSpeed;
    }
}

void Boss::getMovementDirection(float playerX, float playerY, float& outDX, float& outDY) {
    if (currentAction == BOSS_ACTION_CHARGE) {
        outDX = chargeDirectionX;
        outDY = chargeDirectionY;
        return;
    }
    float dx = playerX - (x + width/2);
    float dy = playerY - (y + height/2);
    float len = sqrtf(dx*dx + dy*dy);
    if (len > 0.1f) { outDX = dx/len; outDY = dy/len; }
    else { outDX = 0; outDY = 0; }
}

// ═══════════════════════════════════════════════════════════════
// Damage Handling
// ═══════════════════════════════════════════════════════════════
void Boss::onDamaged(float amount) {
    hitFlashTimer = 8; // Flash white for 8 frames
}

// ═══════════════════════════════════════════════════════════════
// Drawing — 3-Phase Visual Feedback
// ═══════════════════════════════════════════════════════════════
void Boss::draw() {
    float cx = x + width/2, cy = y + height/2, r = width/2;
    float hpR = health / maxHealth;
    float fa = angle * 3.14159f / 180.0f;

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ── Hit Flash: override all colors to white briefly ──
    bool flashing = (hitFlashTimer > 0);

    // ── Phase-dependent visual aura ──
    float pulseR = r + 10 + 6*sinf(bossAngle*0.06f);

    if (currentPhase == BOSS_PHASE_DESPERATION) {
        // Desperate purple/black pulsing aura
        float pulse = 0.5f + 0.3f * sinf(bossAngle * 0.12f);
        glColor4f(0.6f * pulse, 0.0f, 0.5f * pulse, 0.6f);
        bossCircle(cx, cy, pulseR + 15, 32);
    }
    else if (currentPhase == BOSS_PHASE_ENRAGE) {
        // Enraged fiery aura
        float pulse = 0.5f + 0.5f * sinf(bossAngle * 0.1f);
        glColor4f(1.0f, 0.3f * pulse, 0.0f, 0.5f);
        bossCircle(cx, cy, pulseR + 10, 32);
    }
    else {
        // Phase 1: Subtle dark red aura
        glColor4f(0.5f, 0.05f, 0.08f, 0.4f);
        bossCircle(cx, cy, pulseR, 32);
    }

    // ── Main Body ──
    if (flashing) {
        glColor3f(1.0f, 1.0f, 1.0f); // White flash
    }
    else if (currentAction == BOSS_ACTION_CHARGE) {
        glColor3f(1.0f, 0.5f, 0.0f); // Orange during charge
    }
    else if (currentAction == BOSS_ACTION_ROAR) {
        float rs = roarScale;
        glColor3f(0.8f, 0.1f, 0.0f);
        // Draw body scaled up during roar
        float bodyR = r * rs;
        glBegin(GL_POLYGON);
        for(int i=0;i<8;i++){
            float a=bossAngle*3.14159f/180.0f + i*6.2832f/8;
            glVertex2f(cx+bodyR*cosf(a), cy+bodyR*sinf(a));
        }
        glEnd();
        // Draw roar shockwave rings
        glColor4f(1.0f, 0.2f, 0.0f, 0.3f);
        float ringR = r * 1.5f + actionTimer * 2.0f;
        bossCircleOutline(cx, cy, ringR, 24);
        glColor4f(1.0f, 0.4f, 0.1f, 0.15f);
        bossCircleOutline(cx, cy, ringR * 0.7f, 24);

        goto draw_eyes; // Skip normal body drawing
    }
    else if (currentAction == BOSS_ACTION_STUN) {
        // Dazed: flickering between body color and grey
        float flicker = (actionTimer % 6 < 3) ? 0.5f : 0.3f;
        glColor3f(flicker, flicker, flicker);
    }
    else if (currentPhase == BOSS_PHASE_DESPERATION) {
        glColor3f(0.4f, 0.0f, 0.35f); // Dark purple body
    }
    else if (currentPhase == BOSS_PHASE_ENRAGE) {
        glColor3f(0.7f, 0.15f, 0.05f); // Deep red body
    }
    else {
        glColor3f(0.6f, 0.04f, 0.04f); // Normal red body
    }

    // Rotating octagon body
    glBegin(GL_POLYGON);
    for(int i=0;i<8;i++){
        float a=bossAngle*3.14159f/180.0f + i*6.2832f/8;
        glVertex2f(cx+r*cosf(a), cy+r*sinf(a));
    }
    glEnd();

    // Inner spikes
    if (!flashing) {
        if (currentPhase == BOSS_PHASE_DESPERATION)
            glColor3f(0.25f, 0.0f, 0.2f);
        else
            glColor3f(0.35f, 0.0f, 0.0f);
    }
    glBegin(GL_POLYGON);
    for(int i=0;i<8;i++){
        float a=(bossAngle+22.5f)*3.14159f/180.0f + i*6.2832f/8;
        glVertex2f(cx+r*0.7f*cosf(a), cy+r*0.7f*sinf(a));
    }
    glEnd();

    // Shoulder armor
    if (!flashing) {
        if (currentPhase == BOSS_PHASE_ENRAGE) glColor3f(0.6f, 0.2f, 0.1f);
        else if (currentPhase == BOSS_PHASE_DESPERATION) glColor3f(0.35f, 0.1f, 0.3f);
        else glColor3f(0.45f, 0.08f, 0.08f);
    }
    bossCircle(cx-sinf(fa)*r*0.6f, cy+cosf(fa)*r*0.6f, r*0.35f, 12);
    bossCircle(cx+sinf(fa)*r*0.6f, cy-cosf(fa)*r*0.6f, r*0.35f, 12);

draw_eyes:
    // ── Eyes ──
    float eyeOff = r * 0.28f;
    if (currentPhase == BOSS_PHASE_DESPERATION) {
        // Purple glowing eyes
        glColor3f(0.9f, 0.1f, 0.9f);
    } else if (currentPhase == BOSS_PHASE_ENRAGE) {
        // Bright orange-yellow rage eyes
        glColor3f(1.0f, 0.6f, 0.0f);
    } else {
        glColor3f(1.0f, 0.8f, 0.0f);
    }
    bossCircle(cx+cosf(fa)*eyeOff-sinf(fa)*8, cy+sinf(fa)*eyeOff+cosf(fa)*8, r*0.15f, 16);
    bossCircle(cx+cosf(fa)*eyeOff+sinf(fa)*8, cy+sinf(fa)*eyeOff-cosf(fa)*8, r*0.15f, 16);
    // Pupils
    glColor3f(0.0f, 0.0f, 0.0f);
    bossCircle(cx+cosf(fa)*eyeOff-sinf(fa)*8, cy+sinf(fa)*eyeOff+cosf(fa)*8, r*0.07f, 12);
    bossCircle(cx+cosf(fa)*eyeOff+sinf(fa)*8, cy+sinf(fa)*eyeOff-cosf(fa)*8, r*0.07f, 12);

    // ── Mouth — jagged teeth ──
    if (currentAction == BOSS_ACTION_ROAR) {
        // Wide open mouth during roar
        glColor3f(0.1f, 0.0f, 0.0f);
        bossCircle(cx+cosf(fa)*r*0.35f, cy+sinf(fa)*r*0.35f, r*0.25f, 12);
        glColor3f(1.0f, 0.85f, 0.0f);
        glLineWidth(3.0f);
    } else {
        glColor3f(1.0f, 0.85f, 0.0f);
        glLineWidth(2.5f);
    }
    glBegin(GL_LINE_STRIP);
    for(int i=0;i<=8;i++){
        float off=(i-4)*r*0.09f;
        float fwd=r*0.40f+(i%2==0?r*0.08f:-r*0.08f);
        glVertex2f(cx+cosf(fa)*fwd-sinf(fa)*off, cy+sinf(fa)*fwd+cosf(fa)*off);
    }
    glEnd();

    // ── Phase indicator ring ──
    if (currentPhase == BOSS_PHASE_DESPERATION) {
        glColor4f(0.8f, 0.0f, 0.8f, 0.6f);
        glLineWidth(3.0f);
        bossCircleOutline(cx, cy, r + 5, 24);
    } else if (currentPhase == BOSS_PHASE_ENRAGE) {
        glColor4f(1.0f, 0.4f, 0.0f, 0.6f);
        glLineWidth(3.0f);
        bossCircleOutline(cx, cy, r + 5, 24);
    } else {
        glColor3f(1.0f-hpR, hpR, 0.0f);
        glLineWidth(3.0f);
        bossCircleOutline(cx, cy, r+3, 8);
    }

    // ── Action indicator label ──
    const char* lbl = "BOSS";
    switch (currentAction) {
    case BOSS_ACTION_ROAR:   lbl = "!! ROARING !!"; break;
    case BOSS_ACTION_CHARGE: lbl = "!! CHARGING !!"; break;
    case BOSS_ACTION_STUN:   lbl = "* STUNNED *"; break;
    case BOSS_ACTION_SUMMON: lbl = "!! SUMMONING !!"; break;
    case BOSS_ACTION_THROW:  lbl = "THROWING..."; break;
    default: {
        if (currentPhase == BOSS_PHASE_DESPERATION) lbl = "DESPERATE";
        else if (currentPhase == BOSS_PHASE_ENRAGE) lbl = "ENRAGED";
        else lbl = "BOSS";
        break;
    }
    }
    if (currentPhase == BOSS_PHASE_DESPERATION)
        glColor3f(0.9f, 0.1f, 0.9f);
    else if (currentPhase == BOSS_PHASE_ENRAGE)
        glColor3f(1.0f, 0.5f, 0.1f);
    else
        glColor3f(1.0f, 0.15f, 0.15f);

    glRasterPos2f(cx-30, cy-r-16);
    for(const char* c=lbl;*c;c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12,(int)*c);

    // ── Draw Projectiles ──
    for (auto& p : projectiles) {
        if (!p.active) continue;
        // Dark grey boulder
        glColor3f(0.4f, 0.35f, 0.3f);
        bossCircle(p.x, p.y, p.radius, 8);
        // Jagged rock edges
        glColor3f(0.25f, 0.2f, 0.18f);
        bossCircleOutline(p.x, p.y, p.radius, 8);
        // Trail
        glColor4f(0.5f, 0.3f, 0.1f, 0.3f);
        bossCircle(p.x - p.vx*2, p.y - p.vy*2, p.radius*0.6f, 6);
    }

    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glPopMatrix();
}

#pragma once
#include "Enemy.h"
#include <cmath>
#include <vector>

// ─── Boss Projectile (thrown boulder/debris) ───
struct BossProjectile {
    float x, y;
    float vx, vy;
    float radius;
    float damage;
    int   lifetime;     // frames remaining
    bool  active;

    BossProjectile() : x(0),y(0),vx(0),vy(0),radius(18),damage(35),lifetime(120),active(false) {}
};

// ─── Boss Phase FSM ───
enum BossPhase {
    BOSS_PHASE_CHASE      = 0,  // Phase 1: Chase + throw projectiles
    BOSS_PHASE_ENRAGE     = 1,  // Phase 2 (50% HP): Roar then straight-line charges
    BOSS_PHASE_DESPERATION = 2  // Phase 3 (15% HP): Summon swarm, try to corner player
};

enum BossAction {
    BOSS_ACTION_IDLE,
    BOSS_ACTION_CHASE,
    BOSS_ACTION_THROW,       // Phase 1: Winding up to throw
    BOSS_ACTION_ROAR,        // Phase 2: Transition roar animation
    BOSS_ACTION_CHARGE,      // Phase 2: Straight-line destructive charge
    BOSS_ACTION_SUMMON,      // Phase 3: Summoning swarm
    BOSS_ACTION_STUN         // Briefly stunned after hitting a wall during charge
};

class Boss : public Enemy {
private:
    // ─── Phase State Machine ───
    BossPhase  currentPhase;
    BossAction currentAction;
    int        actionTimer;         // Frames in current action

    // ─── Phase 1: Chase & Throw ───
    float throwCooldownMax;
    float throwCooldownTimer;
    float throwWindupFrames;        // Frames to windup before releasing

    // ─── Phase 2: Enrage ───
    bool  hasRoared;                // One-time roar transition
    float enrageSpeedMult;
    float chargeDirectionX;
    float chargeDirectionY;
    float chargeDuration;
    float chargeTimer;
    float chargeCooldownTimer;
    float chargeCooldownMax;

    // ─── Phase 3: Desperation ───
    bool  hasSummonedSwarm;
    int   swarmCount;

    // ─── Visual ───
    float bossAngle;                // Rotation for visual effects
    float roarScale;                // Grows during roar animation
    int   hitFlashTimer;            // White flash when taking damage

    // ─── Projectiles ───
    std::vector<BossProjectile> projectiles;

    // ─── Screen Shake (reported to Game) ───
    float shakeIntensity;

public:
    Boss(float x, float y, char* tex, unsigned int difficulty);
    void draw() override;

    // Core update — call every frame from Game::timer
    void update(float playerX, float playerY, int screenW, int screenH);

    // Phase transition checks
    void checkPhaseTransition();

    // Projectile management
    void throwProjectile(float playerX, float playerY);
    void updateProjectiles();
    std::vector<BossProjectile>& getProjectiles() { return projectiles; }

    // Getters for Game integration
    BossPhase  getPhase()  const { return currentPhase; }
    BossAction getAction() const { return currentAction; }
    float getShakeIntensity() const { return shakeIntensity; }
    void  resetShake() { shakeIntensity = 0; }
    bool  needsSwarmSpawn() const { return currentPhase == BOSS_PHASE_DESPERATION && !hasSummonedSwarm; }
    void  markSwarmSpawned() { hasSummonedSwarm = true; }
    int   getSwarmCount() const { return swarmCount; }
    void  onDamaged(float amount);   // Triggers hit flash
    bool  isKnockbackImmune() const { return true; }  // Boss NEVER gets pushed

    // Effective movement
    float getEffectiveSpeed();
    void  getMovementDirection(float playerX, float playerY, float& outDX, float& outDY);
};

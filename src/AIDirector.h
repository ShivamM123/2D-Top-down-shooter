#pragma once
#include <vector>

class Game;
class Player;
class Enemy;

enum DirectorPhase {
    PHASE_BUILD_UP,
    PHASE_PEAK,
    PHASE_RELAX
};

class AIDirector {
private:
    DirectorPhase currentPhase;
    float stressLevel;       // 0.0 to 1.0
    int phaseTimer;
    
    // Stress factors
    float lastPlayerHealth;
    int timeInCombat;
    int shotsFired;

    int hordeSpawnCounter;
    int maxHordeSize;

    void updateStress(Player* player, const std::vector<Enemy*>& enemies);
    void handleBuildUp(Game* game);
    void handlePeak(Game* game);
    void handleRelax(Game* game);

public:
    AIDirector();
    ~AIDirector();

    void update(Game* game, Player* player, const std::vector<Enemy*>& enemies);
    float getStressLevel() const { return stressLevel; }
    DirectorPhase getPhase() const { return currentPhase; }
    void reportShotFired() { shotsFired++; }
};

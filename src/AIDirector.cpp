#include "AIDirector.h"
#include "Game.h"
#include "Player.h"
#include "Enemy.h"
#include <cmath>
#include <iostream>

AIDirector::AIDirector() {
    currentPhase = PHASE_BUILD_UP;
    stressLevel = 0.0f;
    phaseTimer = 0;
    lastPlayerHealth = 100.0f;
    timeInCombat = 0;
    shotsFired = 0;
    hordeSpawnCounter = 0;
    maxHordeSize = 15;
}

AIDirector::~AIDirector() {
}

void AIDirector::updateStress(Player* player, const std::vector<Enemy*>& enemies) {
    // Calculate stress based on player health lost
    float currentHealth = player->getHealth();
    if (currentHealth < lastPlayerHealth) {
        stressLevel += (lastPlayerHealth - currentHealth) * 0.02f; // Increase stress on damage
    } else if (currentHealth > lastPlayerHealth) {
        stressLevel -= (currentHealth - lastPlayerHealth) * 0.01f; // Decrease slightly on healing
    }
    lastPlayerHealth = currentHealth;

    // Time in combat (enemies nearby)
    bool inCombat = false;
    for (auto e : enemies) {
        float dx = e->getPositionX() - player->getPositionX();
        float dy = e->getPositionY() - player->getPositionY();
        if (std::sqrt(dx*dx + dy*dy) < 500.0f) {
            inCombat = true;
            break;
        }
    }

    if (inCombat) {
        timeInCombat++;
        stressLevel += 0.0005f; // Gradual increase during combat
    } else {
        timeInCombat = 0;
        stressLevel -= 0.001f; // Gradual decrease when safe
    }

    // Shots fired (simulating ammo depletion)
    if (shotsFired > 0) {
        stressLevel += shotsFired * 0.001f;
        shotsFired = 0; // Reset after accounting
    }

    // Clamp stress
    if (stressLevel < 0.0f) stressLevel = 0.0f;
    if (stressLevel > 1.0f) stressLevel = 1.0f;
}

void AIDirector::update(Game* game, Player* player, const std::vector<Enemy*>& enemies) {
    updateStress(player, enemies);
    phaseTimer++;

    switch (currentPhase) {
    case PHASE_BUILD_UP:
        handleBuildUp(game);
        break;
    case PHASE_PEAK:
        handlePeak(game);
        break;
    case PHASE_RELAX:
        handleRelax(game);
        break;
    }
}

void AIDirector::handleBuildUp(Game* game) {
    // Spawn standard enemies periodically
    if (phaseTimer % 100 == 0) {
        game->SpawnEnemy(1 + (rand() % 2), false); // Spawn 1-2 standard/special enemies
    }

    // Transition logic: Low stress -> start peak (horde)
    if (phaseTimer > 1000 && stressLevel < 0.3f) {
        currentPhase = PHASE_PEAK;
        phaseTimer = 0;
        hordeSpawnCounter = maxHordeSize; // Target horde size
        std::cout << "AI Director: Phase PEAK (Horde Incoming!)" << std::endl;
    } 
    // If stress gets too high during buildup, relax early
    else if (stressLevel > 0.8f) {
        currentPhase = PHASE_RELAX;
        phaseTimer = 0;
        std::cout << "AI Director: Phase RELAX (Player Stressed)" << std::endl;
    }
}

void AIDirector::handlePeak(Game* game) {
    // Rapidly spawn hordes
    if (hordeSpawnCounter > 0) {
        if (phaseTimer % 20 == 0) {
            game->SpawnEnemy(3, true); // Spawn chunks of horde
            hordeSpawnCounter -= 3;
        }
    }

    // Transition logic: Horde over or stress maxed out
    if ((hordeSpawnCounter <= 0 && phaseTimer > 500) || stressLevel > 0.9f) {
        currentPhase = PHASE_RELAX;
        phaseTimer = 0;
        std::cout << "AI Director: Phase RELAX (Horde Over)" << std::endl;
    }
}

void AIDirector::handleRelax(Game* game) {
    // No spawns, give the player a breather

    // Maybe drop a health pack if stress is very high
    if (phaseTimer == 50 && stressLevel > 0.8f) {
        // Find a spot slightly away from player
        // Just triggering drop via game (Game will need a way to do this at random valid coords)
        // For simplicity, skip direct drops, let them loot existing map if we had map items.
    }

    // Transition logic: Give them at least a set time to rest, then back to build up
    if (phaseTimer > 800) {
        currentPhase = PHASE_BUILD_UP;
        phaseTimer = 0;
        std::cout << "AI Director: Phase BUILD_UP" << std::endl;
    }
}

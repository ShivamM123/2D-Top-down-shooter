#pragma once
#include <iostream>
#include <GL/freeglut.h>
#include <vector>
#include "Entity.h"
#include "Player.h"
#include "Bullet.h"
#include "Enemy.h"
#include "Boss.h"
#include "Perk.h"
#include "AIDirector.h"

class Game: IDrawable {
private:
    char* filename;
    char* filename2;
    char* filename3;

    Player* player;
    std::vector<Bullet*> bullets;
    std::vector<Enemy*>  enemies;
    std::vector<Perk*>   perks;
    Boss* boss;
    AIDirector* director;

    int   score;
    int   killCount;
    unsigned int gameMode;
    int   spawnTimer;

    int   activeWeapon;    // 0=pistol 1=rapid 2=flame
    int   weaponTimer;
    bool  shieldActive;
    int   shieldTimer;

    bool  bossAlive;
    float bossMaxHealth;
    int   nextBossKillThreshold;

    float bgAnimTimer;
    int   perkDropCounter;
    int   waveFlashTimer;
    int   damageFlashTimer;

public:
    Game();
    ~Game();
    void reset();
    void setGameMode(unsigned int mode);
    void updateMovement(bool keys[256]);
    void onKeyPressed(unsigned char key, int x, int y);
    void onMouseClicked(int button, int state, int x, int y);
    void onMouseMove(int x, int y);
    void timer(void (*t)(int));
    void draw();

    // Make accessible for AIDirector
    void SpawnEnemy(int count = 1, bool isHorde = false);
    void dropPerk(float x, float y);

private:
    void drawBackground();
    void drawEnemyHealthBars();
    void drawHUD();
    void SpawnBoss();
    void moveEnemy(Enemy* e);
    void fireBullet(float mx, float my);
    void tryPickupPerks();
    bool detectCollision(Entity* a, Entity* b);
    void pushBack(Entity* e1, Entity* e2);
    void clampPlayer();
};

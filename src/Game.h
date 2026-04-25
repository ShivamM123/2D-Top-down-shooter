#include <iostream>
#include <GL/freeglut.h>
#include "Entity.h"
#include "Player.h"
#include "Bullet.h"
#include "Enemy.h"
#include <vector>

class Game: IDrawable {
private:
	char* filename;
	char* filename2;
	char* filename3;
	Player* player;
	std::vector<Bullet*> bullets;
	std::vector<Enemy*> enemies;
	int score;
	unsigned int gameMode;
	int spawnTimer;
public:
	Game();
	void setGameMode(unsigned int mode);
	void onKeyPressed(unsigned char key, int x, int y);
	void updateMovement(bool keys[256]);
	void onMouseClicked(int button, int state, int x, int y);
	void onMouseMove(int x, int y);
	void timer(void (*t)(int));
	void draw();
	void drawHUD();
	bool detectCollision(Entity* entity1, Entity* entity2);
	void SpawnEnemy();
	void moveEnemy(Enemy* enemy);
	void pushBack(Entity* entity1, Entity* entity2);
};

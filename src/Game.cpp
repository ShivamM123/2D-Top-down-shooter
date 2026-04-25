#include "Game.h"
#include <time.h>
#include <string.h>
#include <math.h>

Game::Game() {
	filename = strdup("res/player.bmp");
	filename2 = strdup("res/enemy.bmp");
	filename3 = strdup("res/bullet.bmp");
	player = new Player(100, 100, 5, filename, 100);
	score = 0;
	gameMode = 1;
	spawnTimer = 0;
}

void Game::setGameMode(unsigned int mode)
{
	gameMode = mode;
	int health = 100;
	switch (mode) {
	case 0: health = 120; break;
	case 1: health = 100; break;
	case 2: health = 80; break;
	}
	player->setHealth(health);
	score = 0;
	enemies.clear();
	bullets.clear();
}

// found in OpenGL Game Development by Example just rewrote to be compatible with this program 
void Game::SpawnEnemy() {
	float speed = 1.0f;
	float damage = 10.0f;
	if (gameMode == 0) { speed = 0.5f; damage = 5.0f; }
	else if (gameMode == 2) { speed = 1.5f; damage = 20.0f; }

	Enemy* newEnemy = new Enemy(70, 70, speed, filename2, 100, damage);
	float marginX = newEnemy->getWidth();
	float marginY = newEnemy->getHeight();
	float spawnX = (rand() % (int) (glutGet(GLUT_WINDOW_WIDTH) - (marginX * 2))) + marginX;
	float spawnY = glutGet(GLUT_WINDOW_HEIGHT) - ((rand() % (int) (player->getHeight() - (marginY * 2))) + marginY);
	
	// Ensure enemy doesn't spawn too close to player
	while (abs(spawnX - player->getPositionX()) < 150 && abs(spawnY - player->getPositionY()) < 150) {
		spawnX = (rand() % (int) (glutGet(GLUT_WINDOW_WIDTH) - (marginX * 2))) + marginX;
		spawnY = glutGet(GLUT_WINDOW_HEIGHT) - ((rand() % (int) (player->getHeight() - (marginY * 2))) + marginY);
	}

	newEnemy->setPositionX(spawnX);
	newEnemy->setPositionY(spawnY);
	enemies.push_back(newEnemy);
}


void Game::moveEnemy(Enemy* enemy) {
	float dirX = player->getPositionX() - enemy->getPositionX();
	float dirY = player->getPositionY() - enemy->getPositionY();
	float normalize = sqrt(dirX * dirX + dirY * dirY);
	if (normalize > 0) {
		dirX /= normalize;
		dirY /= normalize;
	}
	enemy->setPositionX(enemy->getPositionX() + dirX * enemy->getMoveSpeed());
	enemy->setPositionY(enemy->getPositionY() + dirY * enemy->getMoveSpeed());

	double angle = atan2(dirY, dirX) * 180 / 3.15;
	enemy->setAngle(angle);
}

void Game::updateMovement(bool keys[256]) {
	if (keys['w'] || keys['W']) player->moveUP();
	if (keys['s'] || keys['S']) player->moveDown();
	if (keys['a'] || keys['A']) player->moveLeft();
	if (keys['d'] || keys['D']) player->moveRight();
}

void Game::onKeyPressed(unsigned char key, int x, int y) {
	switch(key) {
	case 033:
		glutLeaveMainLoop();
		break;
	}
}

void Game::onMouseClicked(int button, int state, int x, int y) {
	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		Bullet* newBullet = new Bullet(10, 10, 20, filename3, 0, 20);
		newBullet->setPositionX(player->getPositionX());
		newBullet->setPositionY(player->getPositionY());
		double angle = atan2((float) y - player->getPositionY(), (float) x - player->getPositionX());
		newBullet->setAngle(angle);
		bullets.push_back(newBullet);
	}
}

void Game::onMouseMove(int x, int y) {
	float angle = atan2((float) y - player->getPositionY(), (float) x - player->getPositionX()) * 180 / 3.15;
	player->setAngle(angle);
}

void Game::timer(void(*t)(int)) {
	spawnTimer++;
	int spawnRate = 200;
	if (gameMode == 0) spawnRate = 300;
	else if (gameMode == 2) spawnRate = 100;

	if (spawnTimer >= spawnRate) {
		SpawnEnemy();
		spawnTimer = 0;
	}

	for (auto it = bullets.begin(); it != bullets.end(); ) {
		(*it)->moveBullet();
		if((*it)->getPositionX() < 0 || (*it)->getPositionX() > glutGet(GLUT_WINDOW_WIDTH) || 
		   (*it)->getPositionY() < 0 || (*it)->getPositionY() > glutGet(GLUT_WINDOW_HEIGHT)) {
			delete *it;
			it = bullets.erase(it);
		} else {
			++it;
		}
	}

	for (auto eIt = enemies.begin(); eIt != enemies.end(); ) {
		moveEnemy(*eIt);
		bool enemyDied = false;

		for (auto bIt = bullets.begin(); bIt != bullets.end(); ) {
			if(detectCollision(*eIt, *bIt)) {
				(*eIt)->setHealth((*eIt)->getHealth() - (*bIt)->getDamage());
				pushBack(*bIt, *eIt);
				delete *bIt;
				bIt = bullets.erase(bIt);
				if((*eIt)->getHealth() <= 0) {
					delete *eIt;
					eIt = enemies.erase(eIt);
					enemyDied = true;
					score += 10;
					break;
				}
			} else {
				++bIt;
			}
		}

		if (!enemyDied) {
			if(detectCollision(player, *eIt)) {
				player->setHealth(player->getHealth() - (*eIt)->getDamage());
				pushBack(*eIt, player);
				if(player->getHealth() <= 0) {
					exit(0); // Game Over (For now just exit)
				}
			}
			++eIt;
		}
	}

	glutPostRedisplay();
	glutTimerFunc(10, t, 0); // Smooth 10ms timer
}

void Game::drawHUD() {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 0, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Draw Score
	glColor3f(1.0, 1.0, 1.0);
	char scoreText[50];
	sprintf(scoreText, "SCORE: %d", score);
	glRasterPos2f(20, 30);
	for (char* c = scoreText; *c != '\0'; c++) {
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
	}

	// Draw Health Bar
	float hpPercent = player->getHealth() / 100.0f; // Simplified
	if (hpPercent > 1.0f) hpPercent = 1.0f;
	if (hpPercent < 0.0f) hpPercent = 0.0f;
	
	glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_QUADS);
	glVertex2f(20, 50);
	glVertex2f(20 + (200 * hpPercent), 50);
	glVertex2f(20 + (200 * hpPercent), 70);
	glVertex2f(20, 70);
	glEnd();

	glColor3f(1.0, 1.0, 1.0);
	glBegin(GL_LINE_LOOP);
	glVertex2f(20, 50);
	glVertex2f(220, 50);
	glVertex2f(220, 70);
	glVertex2f(20, 70);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void Game::draw() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	player->draw();
	for (auto b : bullets) {
		b->draw();
	}
	for (auto e : enemies) {
		e->draw();
	}
	drawHUD();
	glutSwapBuffers();
}

// AABB (axis-aligned bounding box) collision
bool Game::detectCollision(Entity* entity1, Entity* entity2) {
	if(entity2 == nullptr) {
		return false;
	}
	bool collisionX = entity1->getPositionX() + entity1->getWidth() >= entity2->getPositionX() &&
		entity2->getPositionX() + entity2->getWidth() >= entity1->getPositionX();
	bool collisionY = entity1->getPositionY() + entity1->getHeight() >= entity2->getPositionY() &&
		entity2->getPositionY() + entity2->getHeight() >= entity1->getPositionY();

	return collisionX && collisionY;
}

void Game::pushBack(Entity* entity1, Entity* entity2) {
	// push back to upper left corner
	if(entity1->getPositionX() > entity2->getPositionX() && entity1->getPositionY() > entity2->getPositionY()) {
		entity2->setPositionX(entity2->getPositionX() - 30);
		entity2->setPositionY(entity2->getPositionY() - 30);
	}
	// puch back to lower left corner
	if(entity1->getPositionX() > entity2->getPositionX() && entity1->getPositionY() < entity2->getPositionY()) {
		entity2->setPositionX(entity2->getPositionX() - 30);
		entity2->setPositionY(entity2->getPositionY() + 30);
	}
	// push back to upper right corner
	if(entity1->getPositionX() < entity2->getPositionX() && entity1->getPositionY() > entity2->getPositionY()) {
		entity2->setPositionX(entity2->getPositionX() + 30);
		entity2->setPositionY(entity2->getPositionY() - 30);
	}
	// push back to lower right corner
	if(entity1->getPositionX() < entity2->getPositionX() && entity1->getPositionY() < entity2->getPositionY()) {
		entity2->setPositionX(entity2->getPositionX() + 30);
		entity2->setPositionY(entity2->getPositionY() + 30);
	}
	// push up
	if(entity1->getPositionX() == entity2->getPositionX() && entity1->getPositionY() > entity2->getPositionY()) {
		entity2->setPositionY(entity2->getPositionY() - 30);
	}

	// push down
	if(entity1->getPositionX() == entity2->getPositionX() && entity1->getPositionY() < entity2->getPositionY()) {
		entity2->setPositionY(entity2->getPositionY() + 30);
	}

	// push right
	if(entity1->getPositionX() < entity2->getPositionX() && entity1->getPositionY() == entity2->getPositionY()) {
		entity2->setPositionX(entity2->getPositionX() + 30);
	}

	// push left
	if(entity1->getPositionX() > entity2->getPositionX() && entity1->getPositionY() == entity2->getPositionY()) {
		entity2->setPositionX(entity2->getPositionX() - 30);
	}
}
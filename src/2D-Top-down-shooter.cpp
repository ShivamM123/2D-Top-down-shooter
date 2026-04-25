#include <GL/freeglut.h>
#include <iostream>
#include <cmath>
#include "MenuObject.h"
#include "Game.h"

#define _WIN32_WINNT 0x500

MenuObject menu;
Game* game = new Game();
bool keys[256];

void displayMenu(void) {
	menu.draw();
	glutSwapBuffers();
}

void displayGame(void) {
	game->draw();
}

void startNewGame();
void startSettings();
void MouseShoot(int, int, int, int);
void keyboardDown(unsigned char, int, int);
void keyboardUp(unsigned char, int, int);
void mouseMove(int, int);
void reshape(int, int);
void timer(int);
void onMouseMenu(int button, int state, int x, int y);

int main(int argc, char** argv) {
	for(int i=0; i<256; i++) keys[i] = false;
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_STENCIL);
	glutInitWindowSize(600, 500);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Hello world :D");
	glutDisplayFunc(displayMenu);
	glutMouseFunc(onMouseMenu);
	glutMainLoop();
	return 0;
}

void MouseShoot(int button, int state, int x, int y) {
	game->onMouseClicked(button, state, x, y);
}

void onMouseMenu(int button, int state, int x, int y) {
	if (state != GLUT_DOWN)
		return;

	int window_width = glutGet(GLUT_WINDOW_WIDTH);
	int window_height = glutGet(GLUT_WINDOW_HEIGHT);

	GLuint index;

	glReadPixels(x, window_height - y - 1, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &index);

	int selected = index;
	switch (selected) {
		case 1: {
			startNewGame();
			break;
		}
		case 2: {
			startSettings();
			break;
		}
		case 3: {
			exit(0);
		}
	}
	glutPostRedisplay();
}

void onMouseSettings(int button, int state, int x, int y){
	if (state != GLUT_DOWN)
		return;

	int window_width = glutGet(GLUT_WINDOW_WIDTH);
	int window_height = glutGet(GLUT_WINDOW_HEIGHT);

	GLuint index;

	glReadPixels(x, window_height - y - 1, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &index);

	int selected = index;
	switch (selected) {
		case 1: {
			menu.changeGameMode();
			menu.drawSettings();
		} break;
		case 2: {
			glutDisplayFunc(displayMenu);
			glutMouseFunc(onMouseMenu);
		}
	}

	game->setGameMode(menu.getGameMode());
	glutPostRedisplay();
}

void keyboardDown(unsigned char key, int x, int y) {
	keys[key] = true;
	game->onKeyPressed(key, x, y);
}

void keyboardUp(unsigned char key, int x, int y) {
	keys[key] = false;
}

void mouseMove(int x, int y) {
	game->onMouseMove(x, y);
}

void displaySettings() {
	menu.drawSettings();
	glutSwapBuffers();
}

void startSettings() {
	glutDisplayFunc(displaySettings);
	glutMouseFunc(onMouseSettings);
}

// initialize Game functions such as Mouse and Keyboard, change the menu draw function
// into game draw function
void startNewGame() {
	glutDisplayFunc(displayGame);
	glutPassiveMotionFunc(mouseMove);
	glutKeyboardFunc(keyboardDown);
	glutKeyboardUpFunc(keyboardUp);
	glutReshapeFunc(reshape);
	glutTimerFunc(1, timer, 0);
	glutMouseFunc(MouseShoot);
	glutFullScreen();
}


void reshape(int width, int height) {
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
}

void timer(int value) {
	game->updateMovement(keys);
	game->timer(timer);
}
#include <GL/freeglut.h>
#include <iostream>
#include <cmath>
#include "MenuObject.h"
#include "Game.h"

MenuObject menu;
Game* game = new Game();
bool keys[256];

// ─────────────────────────────────────────────
// Game state
// ─────────────────────────────────────────────
enum AppState { STATE_MENU, STATE_SETTINGS, STATE_GAME };
AppState appState = STATE_MENU;

// Forward declarations
void displayMenu();
void displaySettings();
void displayGame();
void startNewGame();
void startSettings();
void returnToMenu();
void MouseShoot(int, int, int, int);
void keyboardDown(unsigned char, int, int);
void keyboardUp(unsigned char, int, int);
void mouseMove(int, int);
void mouseMenuMove(int, int);
void reshape(int, int);
void timer(int);
void menuTimer(int);
void onMouseMenu(int, int, int, int);
void onMouseSettings(int, int, int, int);

// ─────────────────────────────────────────────
// Main
// ─────────────────────────────────────────────
int main(int argc, char** argv) {
    for (int i = 0; i < 256; i++) keys[i] = false;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_STENCIL | GLUT_DEPTH);
    glutInitWindowSize(900, 650);
    glutInitWindowPosition(80, 60);
    glutCreateWindow("2D Top-Down Shooter");

    glutDisplayFunc(displayMenu);
    glutMouseFunc(onMouseMenu);
    glutPassiveMotionFunc(mouseMenuMove);
    glutTimerFunc(16, menuTimer, 0);  // ~60fps menu animation
    glutMainLoop();
    return 0;
}

// ─────────────────────────────────────────────
// Display callbacks
// ─────────────────────────────────────────────
void displayMenu() {
    menu.draw();
    glutSwapBuffers();
}

void displaySettings() {
    menu.drawSettings();
    glutSwapBuffers();
}

void displayGame() {
    game->draw();
}

// ─────────────────────────────────────────────
// Input — Menu
// ─────────────────────────────────────────────
void mouseMenuMove(int x, int y) {
    // Detect hover for glow effect
    int W = glutGet(GLUT_WINDOW_WIDTH);
    int H = glutGet(GLUT_WINDOW_HEIGHT);
    float bw = 240, bh = 48;
    float bx = W/2.0f - bw/2.0f;
    float by = H*0.40f;
    float gap = 64;
    int hover = -1;
    if (x >= bx && x <= bx+bw) {
        if      (y >= by      && y <= by+bh)      hover = 1;
        else if (y >= by+gap  && y <= by+gap+bh)  hover = 2;
        else if (y >= by+gap*2&& y <= by+gap*2+bh)hover = 3;
    }
    menu.setHoverItem(hover);
    glutPostRedisplay();
}

void onMouseMenu(int button, int state, int x, int y) {
    if (state != GLUT_DOWN) return;

    int H = glutGet(GLUT_WINDOW_HEIGHT);
    GLuint index;
    glReadPixels(x, H - y - 1, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &index);

    switch (index) {
        case 1: startNewGame();   break;
        case 2: startSettings();  break;
        case 3: exit(0);
    }
    glutPostRedisplay();
}

void onMouseSettings(int button, int state, int x, int y) {
    if (state != GLUT_DOWN) return;

    int H = glutGet(GLUT_WINDOW_HEIGHT);
    GLuint index;
    glReadPixels(x, H - y - 1, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &index);

    if (index == 1) {
        menu.changeGameMode();
        glutPostRedisplay();
    } else if (index == 2) {
        returnToMenu();
    }
    game->setGameMode(menu.getGameMode());
}

// ─────────────────────────────────────────────
// Input — Game
// ─────────────────────────────────────────────
void keyboardDown(unsigned char key, int x, int y) {
    keys[key] = true;
    game->onKeyPressed(key, x, y);
    // ESC returns to menu instead of quitting directly
    if (key == 27) returnToMenu();
}

void keyboardUp(unsigned char key, int x, int y) {
    keys[key] = false;
}

void MouseShoot(int button, int state, int x, int y) {
    game->onMouseClicked(button, state, x, y);
}

void mouseMove(int x, int y) {
    game->onMouseMove(x, y);
}

// ─────────────────────────────────────────────
// State transitions
// ─────────────────────────────────────────────
void startNewGame() {
    // Reset game with chosen difficulty
    game->setGameMode(menu.getGameMode());

    glutDisplayFunc(displayGame);
    glutPassiveMotionFunc(mouseMove);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutReshapeFunc(reshape);
    glutTimerFunc(10, timer, 0);
    glutMouseFunc(MouseShoot);
    glutFullScreen();
    appState = STATE_GAME;
}

void startSettings() {
    glutDisplayFunc(displaySettings);
    glutMouseFunc(onMouseSettings);
    glutPassiveMotionFunc(nullptr);
    appState = STATE_SETTINGS;
    glutPostRedisplay();
}

void returnToMenu() {
    glutDisplayFunc(displayMenu);
    glutMouseFunc(onMouseMenu);
    glutPassiveMotionFunc(mouseMenuMove);
    glutKeyboardFunc(nullptr);
    glutKeyboardUpFunc(nullptr);
    glutReshapeFunc(nullptr);
    appState = STATE_MENU;

    // Restore window size
    glutReshapeWindow(900, 650);
    glutSetWindowTitle("2D Top-Down Shooter");
    glutTimerFunc(16, menuTimer, 0); // restart menu animation
    glutPostRedisplay();
}

void menuTimer(int) {
    if (appState == STATE_MENU || appState == STATE_SETTINGS)
        glutPostRedisplay();
    if (appState == STATE_MENU || appState == STATE_SETTINGS)
        glutTimerFunc(16, menuTimer, 0);
}

// ─────────────────────────────────────────────
// Reshape & Timer
// ─────────────────────────────────────────────
void reshape(int width, int height) {
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}

void timer(int value) {
    if (appState == STATE_GAME) {
        game->updateMovement(keys);
        game->timer(timer);
    }
}
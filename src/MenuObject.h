#pragma once
#include "IDrawable.h"
#include <GL/freeglut.h>
#include <cstring>
#include <ctime>

#define NUM_STARS 120

class MenuObject: public IDrawable {
private:
    float animTimer;
    int   hoverItem;
    unsigned int gameMode = 1; // 0 easy, 1 normal, 2 hard

    // Star field
    float starX[NUM_STARS];
    float starY[NUM_STARS];
    float starBright[NUM_STARS];

    void drawButton(int stencilVal, const char* label,
                    float x, float y, float w, float h,
                    float r, float g, float b, bool glow);

public:
    MenuObject();
    void draw()     override;
    void drawSettings();
    void changeGameMode();
    unsigned int getGameMode();
    void setHoverItem(int item);
};
#include "MenuObject.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>

// ─────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────
static void fillRect(float x,float y,float w,float h){
    glBegin(GL_QUADS);
    glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);
    glEnd();
}
static void outlineRect(float x,float y,float w,float h){
    glBegin(GL_LINE_LOOP);
    glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);
    glEnd();
}
static void renderStr(float x,float y,void* font,float r,float g,float b,const char* str){
    glColor3f(r,g,b);
    glRasterPos2f(x,y);
    for(const char* c=str;*c;c++) glutBitmapCharacter(font,(int)*c);
}

MenuObject::MenuObject() : animTimer(0.0f), hoverItem(-1), gameMode(1) {
    srand(12345);
    for(int i=0;i<NUM_STARS;i++){
        starX[i]=(float)(rand()%800);
        starY[i]=(float)(rand()%600);
        starBright[i]=0.3f+0.7f*(rand()%100)/100.0f;
    }
    srand((unsigned)time(nullptr));
}

// ─────────────────────────────────────────────
// Button — uses only glColor3f (no alpha needed)
// ─────────────────────────────────────────────
void MenuObject::drawButton(int stencilVal, const char* label,
                             float x, float y, float w, float h,
                             float r, float g, float b, bool glow)
{
    // Shadow
    glColor3f(0.02f,0.02f,0.02f);
    fillRect(x+5,y+5,w,h);

    // Glow border when hovered
    if(glow){
        glColor3f(r+0.25f, g+0.25f, b+0.25f);
        glLineWidth(3.0f);
        outlineRect(x-2,y-2,w+4,h+4);
        glLineWidth(1.0f);
    }

    // Button top half (lighter)
    glColor3f(r, g, b);
    fillRect(x, y, w, h*0.5f);
    // Button bottom half (darker)
    glColor3f(r*0.6f, g*0.6f, b*0.6f);
    fillRect(x, y+h*0.5f, w, h*0.5f);

    // Border
    glColor3f(r+0.3f<1?r+0.3f:1, g+0.3f<1?g+0.3f:1, b+0.3f<1?b+0.3f:1);
    glLineWidth(2.0f);
    outlineRect(x,y,w,h);
    glLineWidth(1.0f);

    // Label — centered approx
    int len = (int)strlen(label);
    float textX = x + w/2.0f - len*5.4f;
    float textY = y + h*0.62f;
    // shadow
    glColor3f(0,0,0);
    glRasterPos2f(textX+1,textY+1);
    for(const char* c=label;*c;c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,(int)*c);
    // text
    glColor3f(1,1,1);
    glRasterPos2f(textX,textY);
    for(const char* c=label;*c;c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,(int)*c);

    // Stencil stamp — invisible quad over the whole button
    glStencilFunc(GL_ALWAYS, stencilVal, 0xFF);
    glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
    fillRect(x,y,w,h);
    glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
}

// ─────────────────────────────────────────────
// Main Menu
// ─────────────────────────────────────────────
void MenuObject::draw() {
    animTimer += 0.025f;
    int W = glutGet(GLUT_WINDOW_WIDTH);
    int H = glutGet(GLUT_WINDOW_HEIGHT);
    if(W<10||H<10) return;

    glClearColor(0.04f,0.06f,0.10f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(0,W,H,0,-1,1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    // ── Stars ──
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for(int i=0;i<NUM_STARS;i++){
        float b2=starBright[i]*(0.6f+0.4f*sinf(animTimer*1.1f+i*0.41f));
        glColor3f(b2,b2,b2*1.1f);
        glVertex2f(starX[i]*(W/800.0f), starY[i]*(H/600.0f));
    }
    glEnd();
    glPointSize(1.0f);

    // ── Title ──
    float tx = (float)(W/2) - 160;
    float ty = (float)(H)*0.22f;
    // glow (slightly offset in orange)
    glColor3f(0.6f,0.3f,0.0f);
    glRasterPos2f(tx+2,ty+2);
    const char* title="2D TOP-DOWN SHOOTER";
    for(const char* c=title;*c;c++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,(int)*c);
    // main
    renderStr(tx,ty,GLUT_BITMAP_TIMES_ROMAN_24,1.0f,0.78f,0.0f,title);

    // Subtitle
    const char* sub="Survive.  Adapt.  Conquer.";
    float subX = (float)(W/2) - 105;
    renderStr(subX, ty+28, GLUT_BITMAP_HELVETICA_12, 0.55f,0.70f,0.85f, sub);

    // Divider
    glColor3f(0.22f,0.30f,0.40f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f((float)(W/2)-210,ty+44); glVertex2f((float)(W/2)+210,ty+44);
    glEnd();
    glLineWidth(1.0f);

    // ── Buttons ──
    float bw=230, bh=46;
    float bx=(float)(W/2)-bw/2.0f;
    float by=(float)(H)*0.40f;
    float gap=62;

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);

    drawButton(1,"NEW GAME",  bx,by,       bw,bh, 0.14f,0.52f,0.82f, hoverItem==1);
    drawButton(2,"SETTINGS",  bx,by+gap,   bw,bh, 0.14f,0.42f,0.58f, hoverItem==2);
    drawButton(3,"QUIT",      bx,by+gap*2, bw,bh, 0.52f,0.10f,0.10f, hoverItem==3);

    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS,0,0xFF);

    // ── Controls hint ──
    renderStr((float)(W/2)-175,(float)(H-16),
              GLUT_BITMAP_HELVETICA_10,
              0.35f,0.45f,0.55f,
              "WASD: Move   |   Mouse: Aim & Shoot   |   ESC: Menu");
}

// ─────────────────────────────────────────────
// Settings
// ─────────────────────────────────────────────
void MenuObject::drawSettings() {
    int W = glutGet(GLUT_WINDOW_WIDTH);
    int H = glutGet(GLUT_WINDOW_HEIGHT);
    if(W<10||H<10) return;

    glClearColor(0.04f,0.06f,0.10f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(0,W,H,0,-1,1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    // Stars (static)
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for(int i=0;i<NUM_STARS;i++){
        glColor3f(starBright[i]*0.5f,starBright[i]*0.5f,starBright[i]*0.6f);
        glVertex2f(starX[i]*(W/800.0f),starY[i]*(H/600.0f));
    }
    glEnd();
    glPointSize(1.0f);

    float tx=(float)(W/2)-68, ty=(float)(H)*0.22f;
    renderStr(tx,ty,GLUT_BITMAP_TIMES_ROMAN_24,1.0f,0.78f,0.0f,"DIFFICULTY");

    const char* modeName="NORMAL";
    float mr=0.9f,mg=0.85f,mb=0.1f;
    if(gameMode==0){modeName="EASY";  mr=0.2f;mg=0.9f;mb=0.2f;}
    if(gameMode==2){modeName="HARD";  mr=1.0f;mg=0.2f;mb=0.1f;}

    char buf[60];
    sprintf(buf,"Current: %s",modeName);
    float subX=(float)(W/2)-(float)strlen(buf)*5.0f;
    renderStr(subX,ty+28,GLUT_BITMAP_HELVETICA_18,mr,mg,mb,buf);

    glColor3f(0.22f,0.30f,0.40f);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2f((float)(W/2)-200,ty+46); glVertex2f((float)(W/2)+200,ty+46);
    glEnd();
    glLineWidth(1.0f);

    const char* desc=(gameMode==0)?"Slower enemies, more HP, lower damage.":
                     (gameMode==2)?"Fast enemies, high damage. Good luck.":
                                   "Balanced speed and damage.";
    renderStr((float)(W/2)-120,ty+66,GLUT_BITMAP_HELVETICA_12,0.55f,0.65f,0.75f,desc);

    float bw=230,bh=46;
    float bx=(float)(W/2)-bw/2.0f;
    float by=(float)(H)*0.50f;

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
    drawButton(1,modeName,  bx,by,     bw,bh, mr*0.75f,mg*0.75f,mb*0.75f, false);
    drawButton(2,"BACK",    bx,by+64,  bw,bh, 0.14f,0.42f,0.58f,false);
    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS,0,0xFF);
}

void MenuObject::changeGameMode(){ gameMode=(gameMode<2)?gameMode+1:0; }
unsigned int MenuObject::getGameMode(){ return gameMode; }
void MenuObject::setHoverItem(int item){ hoverItem=item; }
#include "Game.h"
#include <ctime>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <algorithm>

Game::Game() {
    srand((unsigned)time(nullptr));
    filename  = strdup("res/player.bmp");
    filename2 = strdup("res/enemy.bmp");
    filename3 = strdup("res/bullet.bmp");
    player = new Player(60, 60, 5, filename, 100);
    boss   = nullptr;
    score=0; killCount=0; gameMode=1; spawnTimer=0;
    activeWeapon=0; weaponTimer=0; shieldActive=false; shieldTimer=0;
    bossAlive=false; bossMaxHealth=500; nextBossKillThreshold=20;
    bgAnimTimer=0.0f; perkDropCounter=0; waveFlashTimer=0;
    damageFlashTimer=0;
    player->setPositionX(400); player->setPositionY(300);
}

Game::~Game() {
    delete player;
    if (boss) delete boss;
    for (auto e:enemies) delete e;
    for (auto b:bullets) delete b;
    for (auto p:perks)   delete p;
}

void Game::reset() {
    score=0; killCount=0; spawnTimer=0;
    activeWeapon=0; weaponTimer=0; shieldActive=false; shieldTimer=0;
    bossAlive=false; bossMaxHealth=500; nextBossKillThreshold=20;
    bgAnimTimer=0.0f; perkDropCounter=0; waveFlashTimer=0; damageFlashTimer=0;
    if (boss) { delete boss; boss=nullptr; }
    for (auto e:enemies) delete e; enemies.clear();
    for (auto b:bullets) delete b; bullets.clear();
    for (auto p:perks)   delete p; perks.clear();
    float hp = (gameMode==0)?140:(gameMode==2)?70:100;
    player->setHealth(hp);
    player->setPositionX(400); player->setPositionY(300);
}

void Game::setGameMode(unsigned int mode) { gameMode=mode; reset(); }

// ── Spawn ──
void Game::SpawnEnemy() {
    int W=glutGet(GLUT_WINDOW_WIDTH), H=glutGet(GLUT_WINDOW_HEIGHT);
    if (W<100||H<100) return;

    // Scale difficulty: more HP/speed/damage as score increases
    float baseSpeed  = (gameMode==0)?0.6f:(gameMode==2)?1.8f:1.1f;
    float baseHp     = (gameMode==0)?150:(gameMode==2)?200:175;
    float baseDamage = (gameMode==0)?8:(gameMode==2)?25:15;
    float spd   = baseSpeed  + score * 0.003f;
    float hp    = baseHp     + score * 0.5f;
    float dmg   = baseDamage + score * 0.05f;

    // Spawn count scales: 1 + 1 per 50 kills
    int count = 1 + killCount/50;
    if (count > 4) count = 4;

    for (int i=0;i<count;i++) {
        Enemy* e = new Enemy(55,55,spd,filename2,hp,dmg);
        float ex,ey; int tries=0;
        do {
            // Spawn from edges
            int side = rand()%4;
            if (side==0) { ex=(float)(rand()%W); ey=-60; }
            else if (side==1) { ex=(float)(rand()%W); ey=(float)(H+60); }
            else if (side==2) { ex=-60; ey=(float)(rand()%H); }
            else { ex=(float)(W+60); ey=(float)(rand()%H); }
            tries++;
        } while (tries<5 &&
                 fabsf(ex-player->getPositionX())<150 &&
                 fabsf(ey-player->getPositionY())<150);
        e->setPositionX(ex); e->setPositionY(ey);
        enemies.push_back(e);
    }
}

void Game::SpawnBoss() {
    int W=glutGet(GLUT_WINDOW_WIDTH);
    if (W<100) return;
    boss = new Boss((float)(W/2-60), 80.0f, filename2, gameMode);
    bossAlive=true; bossMaxHealth=boss->getHealth();
    waveFlashTimer=120; // show "BOSS!" flash
}

// ── Movement ──
void Game::updateMovement(bool keys[256]) {
    float spd = 5.0f + (activeWeapon==1?1.0f:0.0f); // slightly faster with rapid
    player->setMoveSpeed(spd);
    if (keys['w']||keys['W']) player->moveUP();
    if (keys['s']||keys['S']) player->moveDown();
    if (keys['a']||keys['A']) player->moveLeft();
    if (keys['d']||keys['D']) player->moveRight();
    clampPlayer();
}

void Game::clampPlayer() {
    int W=glutGet(GLUT_WINDOW_WIDTH), H=glutGet(GLUT_WINDOW_HEIGHT);
    float px=player->getPositionX(), py=player->getPositionY();
    if (px<0) player->setPositionX(0);
    if (py<0) player->setPositionY(0);
    if (px+player->getWidth()>W)  player->setPositionX((float)(W-player->getWidth()));
    if (py+player->getHeight()>H) player->setPositionY((float)(H-player->getHeight()));
}

void Game::moveEnemy(Enemy* e) {
    float dx=player->getPositionX()-e->getPositionX();
    float dy=player->getPositionY()-e->getPositionY();
    float len=sqrtf(dx*dx+dy*dy);
    if (len>0.1f){dx/=len;dy/=len;}
    float spd=e->getMoveSpeed();
    if (Boss* b=dynamic_cast<Boss*>(e)) {
        b->updateCharge(player->getPositionX(),player->getPositionY());
        spd=b->getEffectiveSpeed();
    }
    e->setPositionX(e->getPositionX()+dx*spd);
    e->setPositionY(e->getPositionY()+dy*spd);
    e->setAngle(atan2f(dy,dx)*180.0f/3.14159f);
}

// ── Input ──
void Game::onKeyPressed(unsigned char key, int x, int y) {
    if (key==27) glutLeaveMainLoop();
}

void Game::fireBullet(float mx, float my) {
    float px=player->getPositionX()+player->getWidth()*0.5f;
    float py=player->getPositionY()+player->getHeight()*0.5f;
    float ang=atan2f(my-py,mx-px);
    if (activeWeapon==2) {
        for (int i=-2;i<=2;i++) bullets.push_back(new Bullet(px,py,ang+i*0.18f,BULLET_FLAME));
    } else if (activeWeapon==1) {
        float perpX=-sinf(ang)*5.0f, perpY=cosf(ang)*5.0f;
        bullets.push_back(new Bullet(px+perpX,py+perpY,ang,BULLET_RAPID));
        bullets.push_back(new Bullet(px-perpX,py-perpY,ang,BULLET_RAPID));
    } else {
        bullets.push_back(new Bullet(px,py,ang,BULLET_PISTOL));
    }
}

void Game::onMouseClicked(int btn, int state, int x, int y) {
    if (btn==GLUT_LEFT_BUTTON&&state==GLUT_DOWN) fireBullet((float)x,(float)y);
}

void Game::onMouseMove(int x, int y) {
    float px=player->getPositionX()+player->getWidth()*0.5f;
    float py=player->getPositionY()+player->getHeight()*0.5f;
    player->setAngle(atan2f((float)y-py,(float)x-px)*180.0f/3.14159f);
}

// ── Perks ──
void Game::dropPerk(float x, float y) {
    PerkType t[]={PERK_HEALTH,PERK_SHIELD,PERK_BETTER_GUN,PERK_FLAMETHROWER};
    perks.push_back(new Perk(x,y,t[rand()%4]));
}

void Game::tryPickupPerks() {
    float px=player->getPositionX(), py=player->getPositionY();
    float pw=player->getWidth(),     ph=player->getHeight();
    for (auto it=perks.begin();it!=perks.end();) {
        Perk* p=*it;
        if (px<p->x+60&&px+pw>p->x&&py<p->y+60&&py+ph>p->y) {
            float maxHp=(gameMode==0)?140:(gameMode==2)?70:100;
            switch(p->type){
            case PERK_HEALTH: { float h=player->getHealth()+40; if(h>maxHp)h=maxHp; player->setHealth(h); break; }
            case PERK_SHIELD:       shieldActive=true;  shieldTimer=1500; break;
            case PERK_BETTER_GUN:   activeWeapon=1; weaponTimer=2000; break;
            case PERK_FLAMETHROWER: activeWeapon=2; weaponTimer=2000; break;
            }
            delete p; it=perks.erase(it);
        } else ++it;
    }
}

bool Game::detectCollision(Entity* a, Entity* b) {
    if (!a||!b) return false;
    return (a->getPositionX()+a->getWidth() >=b->getPositionX()&&
            b->getPositionX()+b->getWidth() >=a->getPositionX()&&
            a->getPositionY()+a->getHeight()>=b->getPositionY()&&
            b->getPositionY()+b->getHeight()>=a->getPositionY());
}

void Game::pushBack(Entity* e1, Entity* e2) {
    float dx=e2->getPositionX()-e1->getPositionX();
    float dy=e2->getPositionY()-e1->getPositionY();
    float len=sqrtf(dx*dx+dy*dy);
    if (len<0.01f){dx=1;dy=0;len=1;}
    e2->setPositionX(e2->getPositionX()+(dx/len)*30);
    e2->setPositionY(e2->getPositionY()+(dy/len)*30);
}

// ── Timer ──
void Game::timer(void(*t)(int)) {
    bgAnimTimer+=0.025f;
    if (weaponTimer>0){weaponTimer--;if(!weaponTimer)activeWeapon=0;}
    if (shieldTimer>0){shieldTimer--;if(!shieldTimer)shieldActive=false;}
    if (waveFlashTimer>0) waveFlashTimer--;
    if (damageFlashTimer>0) damageFlashTimer--;

    // Spawn
    spawnTimer++;
    int spawnRate=(gameMode==0)?150:(gameMode==2)?70:100;
    if (spawnTimer>=spawnRate&&!bossAlive) { SpawnEnemy(); spawnTimer=0; }
    // First enemy immediately
    if (killCount==0&&enemies.empty()&&!bossAlive&&spawnTimer<2) SpawnEnemy();

    // Perk lifetime
    for (auto it=perks.begin();it!=perks.end();) {
        (*it)->update();
        if ((*it)->isExpired()){delete *it;it=perks.erase(it);}
        else ++it;
    }

    // Bullets
    int W=glutGet(GLUT_WINDOW_WIDTH), H=glutGet(GLUT_WINDOW_HEIGHT);
    for (auto it=bullets.begin();it!=bullets.end();) {
        (*it)->moveBullet();
        float bx=(*it)->getPositionX(), by=(*it)->getPositionY();
        if (bx<-50||bx>W+50||by<-50||by>H+50){delete *it;it=bullets.erase(it);}
        else ++it;
    }

    // Boss trigger
    if (killCount>=nextBossKillThreshold&&!bossAlive) {
        for(auto e:enemies)delete e; enemies.clear();
        SpawnBoss(); nextBossKillThreshold+=20;
    }

    // Boss logic
    if (bossAlive&&boss) {
        moveEnemy(boss);
        for (auto bIt=bullets.begin();bIt!=bullets.end();) {
            if (detectCollision(boss,*bIt)) {
                boss->setHealth(boss->getHealth()-(*bIt)->getDamage());
                delete *bIt; bIt=bullets.erase(bIt);
                if (boss->getHealth()<=0) {
                    dropPerk(boss->getPositionX(),boss->getPositionY());
                    score+=150; killCount++;
                    delete boss; boss=nullptr; bossAlive=false;
                    waveFlashTimer=180;
                    break;
                }
            } else ++bIt;
        }
        if (bossAlive&&boss&&detectCollision(player,boss)) {
            if (shieldActive){shieldActive=false;shieldTimer=0;}
            else {
                player->setHealth(player->getHealth()-boss->getDamage()*0.05f);
                damageFlashTimer=8;
                pushBack(boss,player);
                if (player->getHealth()<=0) exit(0);
            }
        }
    }

    // Enemies
    for (auto eIt=enemies.begin();eIt!=enemies.end();) {
        moveEnemy(*eIt);
        bool died=false;
        for (auto bIt=bullets.begin();bIt!=bullets.end();) {
            if (detectCollision(*eIt,*bIt)) {
                (*eIt)->setHealth((*eIt)->getHealth()-(*bIt)->getDamage());
                delete *bIt; bIt=bullets.erase(bIt);
                if ((*eIt)->getHealth()<=0) {
                    float ex=(*eIt)->getPositionX(), ey=(*eIt)->getPositionY();
                    delete *eIt; eIt=enemies.erase(eIt);
                    died=true; score+=10; killCount++; perkDropCounter++;
                    if (perkDropCounter>=5){dropPerk(ex,ey);perkDropCounter=0;}
                    break;
                }
            } else ++bIt;
        }
        if (!died) {
            if (detectCollision(player,*eIt)) {
                if (shieldActive){shieldActive=false;shieldTimer=0;}
                else {
                    player->setHealth(player->getHealth()-(*eIt)->getDamage()*0.03f);
                    damageFlashTimer=6;
                    pushBack(*eIt,player);
                    if (player->getHealth()<=0) exit(0);
                }
            }
            ++eIt;
        }
    }

    tryPickupPerks();
    glutPostRedisplay();
    glutTimerFunc(10,t,0);
}

// ── Drawing helpers ──
static void fillRect(float x,float y,float w,float h){
    glBegin(GL_QUADS); glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h); glEnd();
}
static void outlineRect(float x,float y,float w,float h){
    glBegin(GL_LINE_LOOP); glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h); glEnd();
}
static void renderText(float x,float y,void* font,const char* str){
    glRasterPos2f(x,y);
    for(const char* c=str;*c;c++) glutBitmapCharacter(font,(int)*c);
}
static void renderTextCentered(float cx, float y, void* font, const char* str) {
    int w=0;
    for(const char* c=str;*c;c++) w+=glutBitmapWidth(font,(int)*c);
    renderText(cx-(float)w/2.0f, y, font, str);
}

// ── Background ──
void Game::drawBackground() {
    int W=glutGet(GLUT_WINDOW_WIDTH), H=glutGet(GLUT_WINDOW_HEIGHT);

    // Red damage flash overlay
    if (damageFlashTimer>0) {
        glColor3f(0.4f,0.0f,0.0f);
    } else {
        glColor3f(0.07f,0.09f,0.12f);
    }
    fillRect(0,0,(float)W,(float)H);

    // Grid
    glColor3f(0.12f,0.15f,0.19f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    int gs=64;
    for(int gx=0;gx<=W;gx+=gs){glVertex2f((float)gx,0);glVertex2f((float)gx,(float)H);}
    for(int gy=0;gy<=H;gy+=gs){glVertex2f(0,(float)gy);glVertex2f((float)W,(float)gy);}
    glEnd();

    // Blood spots (fixed positions)
    srand(7777);
    glColor3f(0.20f,0.04f,0.04f);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    for(int i=0;i<80;i++) glVertex2f((float)(rand()%W),(float)(rand()%H));
    glEnd();
    glPointSize(1.0f);
    srand((unsigned)time(nullptr));
}

// ── Enemy health bars ──
void Game::drawEnemyHealthBars() {
    for (auto e : enemies) {
        float ex = e->getPositionX();
        float ey = e->getPositionY();
        float ew = e->getWidth();
        // Default enemy HP is 175 at base; we track by max at creation
        float hp   = e->getHealth();
        float maxHp= (gameMode==0)?150:(gameMode==2)?200:175;
        // clamp
        float pct = hp/maxHp; if(pct>1)pct=1; if(pct<0)pct=0;

        float barW=ew, barH=5;
        float bx=ex, by=ey-10;

        glColor3f(0.4f,0.0f,0.0f);
        fillRect(bx,by,barW,barH);
        float r=(pct<0.5f)?1.0f:2*(1-pct);
        float g=(pct>0.5f)?1.0f:2*pct;
        glColor3f(r,g,0);
        fillRect(bx,by,barW*pct,barH);
        glColor3f(0.6f,0.6f,0.6f);
        outlineRect(bx,by,barW,barH);
    }
}

// ── HUD ──
void Game::drawHUD() {
    int W=glutGet(GLUT_WINDOW_WIDTH), H=glutGet(GLUT_WINDOW_HEIGHT);

    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0,W,H,0,-1,1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    // ── Top-left panel ──
    float panelW=280, panelH=95, panelX=8, panelY=8;
    glColor3f(0.06f,0.08f,0.11f);
    fillRect(panelX,panelY,panelW,panelH);
    glColor3f(0.25f,0.35f,0.50f);
    glLineWidth(2.0f); outlineRect(panelX,panelY,panelW,panelH); glLineWidth(1.0f);

    // Score
    char buf[80];
    glColor3f(1.0f,0.85f,0.15f);
    sprintf(buf,"SCORE  %d",score);
    renderText(panelX+10, panelY+22, GLUT_BITMAP_HELVETICA_18, buf);

    // Kills
    glColor3f(0.75f,0.75f,0.75f);
    sprintf(buf,"KILLS  %d",killCount);
    renderText(panelX+10, panelY+44, GLUT_BITMAP_HELVETICA_12, buf);

    // HP bar
    float maxHp=(gameMode==0)?140:(gameMode==2)?70:100;
    float hpPct=player->getHealth()/maxHp;
    hpPct=hpPct<0?0:(hpPct>1?1:hpPct);
    float bw=panelW-50;

    glColor3f(0.5f,0.5f,0.5f);
    renderText(panelX+10, panelY+63, GLUT_BITMAP_HELVETICA_12, "HP");
    glColor3f(0.25f,0.0f,0.0f);
    fillRect(panelX+32, panelY+53, bw, 12);
    float r2=hpPct<0.5f?1.0f:2*(1-hpPct), g2=hpPct>0.5f?1.0f:2*hpPct;
    glColor3f(r2,g2,0.0f);
    fillRect(panelX+32, panelY+53, bw*hpPct, 12);
    glColor3f(0.6f,0.6f,0.6f);
    outlineRect(panelX+32, panelY+53, bw, 12);

    // Weapon indicator
    const char* wn="PISTOL"; float wr=1,wg=1,wb=0.2f;
    if(activeWeapon==1){wn="RAPID FIRE"; wr=0.1f;wg=0.95f;wb=1.0f;}
    if(activeWeapon==2){wn="FLAMETHROWER"; wr=1.0f;wg=0.4f;wb=0.0f;}
    glColor3f(wr,wg,wb);
    sprintf(buf,"GUN: %s",wn);
    renderText(panelX+10, panelY+84, GLUT_BITMAP_HELVETICA_10, buf);
    if(shieldActive){
        glColor3f(0.3f,0.7f,1.0f);
        renderText(panelX+120, panelY+84, GLUT_BITMAP_HELVETICA_10, "SHIELD");
    }

    // ── Enemy count top-right ──
    glColor3f(0.55f,0.20f,0.20f);
    sprintf(buf,"ENEMIES: %d",(int)enemies.size());
    renderText((float)(W-130),(float)12, GLUT_BITMAP_HELVETICA_12, buf);

    // ── Wave flash (boss incoming / boss dead) ──
    if (waveFlashTimer>0) {
        float alpha=(float)waveFlashTimer/120.0f;
        glColor3f(1.0f*alpha, 0.2f*alpha, 0.2f*alpha);
        if (bossAlive)
            renderTextCentered((float)(W/2),(float)(H/2)-20,GLUT_BITMAP_TIMES_ROMAN_24,"!! BOSS INCOMING !!");
        else
            renderTextCentered((float)(W/2),(float)(H/2)-20,GLUT_BITMAP_TIMES_ROMAN_24,"BOSS DEFEATED!");
    }

    // ── Boss warning (5 kills away) ──
    int bossIn=nextBossKillThreshold-killCount;
    if (bossIn<=5&&bossIn>0&&!bossAlive) {
        float pulse=0.6f+0.4f*sinf(bgAnimTimer*5);
        glColor3f(pulse,0.1f,0.1f);
        sprintf(buf,"BOSS IN %d KILLS",bossIn);
        renderTextCentered((float)(W/2),18,GLUT_BITMAP_HELVETICA_18,buf);
    }

    // ── Boss HP bar (top center) ──
    if (bossAlive&&boss) {
        float bHp=boss->getHealth()/bossMaxHealth;
        if(bHp<0)bHp=0;
        float bBarW=400, bBarH=22;
        float bBarX=(W-bBarW)/2.0f, bBarY=8;

        glColor3f(0.06f,0.02f,0.02f);
        fillRect(bBarX-2,bBarY-2,bBarW+4,bBarH+4);
        glColor3f(0.35f,0.0f,0.0f);
        fillRect(bBarX,bBarY,bBarW,bBarH);
        glColor3f(1.0f-bHp*0.3f,0.1f,0.1f);
        fillRect(bBarX,bBarY,bBarW*bHp,bBarH);
        glColor3f(1.0f,0.4f,0.4f);
        outlineRect(bBarX,bBarY,bBarW,bBarH);
        glColor3f(1.0f,0.8f,0.8f);
        renderTextCentered(bBarX+bBarW/2,bBarY+16,GLUT_BITMAP_HELVETICA_12,"BOSS");
    }

    // ── Controls hint ──
    glColor3f(0.3f,0.38f,0.48f);
    renderText((float)(W-260),(float)(H-10),GLUT_BITMAP_HELVETICA_10,
               "WASD:Move  LMB:Shoot  ESC:Menu");

    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

// ── Main draw ──
void Game::draw() {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    int W=glutGet(GLUT_WINDOW_WIDTH), H=glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(0,W,H,0,-1,1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    drawBackground();
    for(auto p:perks)   p->draw();
    for(auto e:enemies) e->draw();
    if(bossAlive&&boss)  boss->draw();
    drawEnemyHealthBars();  // drawn after sprites, before bullets
    for(auto b:bullets) b->draw();
    player->draw();
    drawHUD();
    glutSwapBuffers();
}
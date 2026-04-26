#include "Game.h"
#include <ctime>
#include <cstring>
#include <cmath>
#include <cstdio>

Game::Game() {
    srand((unsigned)time(nullptr));
    filename=strdup("res/player.bmp"); filename2=strdup("res/enemy.bmp"); filename3=strdup("res/bullet.bmp");
    player=new Player(60,60,5,filename,100); boss=nullptr; director=new AIDirector();
    score=0;killCount=0;gameMode=1;spawnTimer=0;activeWeapon=0;weaponTimer=0;
    shieldActive=false;shieldTimer=0;bossAlive=false;bossMaxHealth=800;
    nextBossKillThreshold=15;bgAnimTimer=0;perkDropCounter=0;waveFlashTimer=0;damageFlashTimer=0;
    player->setPositionX(400);player->setPositionY(300);
}
Game::~Game(){delete player;if(boss)delete boss;for(auto e:enemies)delete e;for(auto b:bullets)delete b;for(auto p:perks)delete p;delete director;}

void Game::reset(){
    score=0;killCount=0;spawnTimer=0;activeWeapon=0;weaponTimer=0;shieldActive=false;shieldTimer=0;
    bossAlive=false;bossMaxHealth=800;nextBossKillThreshold=15;bgAnimTimer=0;perkDropCounter=0;waveFlashTimer=0;damageFlashTimer=0;
    if(boss){delete boss;boss=nullptr;}
    for(auto e:enemies)delete e;enemies.clear();for(auto b:bullets)delete b;bullets.clear();for(auto p:perks)delete p;perks.clear();
    float hp=(gameMode==0)?140:(gameMode==2)?70:100;
    player->setHealth(hp);player->setPositionX(400);player->setPositionY(300);
}
void Game::setGameMode(unsigned int m){gameMode=m;reset();}

void Game::SpawnEnemy(int count, bool isHorde){
    int W=glutGet(GLUT_WINDOW_WIDTH),H=glutGet(GLUT_WINDOW_HEIGHT);
    if(W<100||H<100)return;

    for(int i=0;i<count;i++){
        // Pick zombie type with weighted random
        ZombieType type=ZOMBIE_WALKER;
        if(isHorde) {
            type=ZOMBIE_RUNNER; // Hordes are runners
        } else {
            int roll=rand()%100;
            if(roll<35) type=ZOMBIE_WALKER;
            else if(roll<55) type=ZOMBIE_RUNNER;
            else if(roll<70) type=ZOMBIE_TANK;
            else if(roll<80) type=ZOMBIE_WITCH;
            else if(roll<90) type=ZOMBIE_SPITTER;
            else type=ZOMBIE_LUNGER;
        }

        float spd,hp,dmg,sz;
        switch(type){
        case ZOMBIE_WALKER:  spd=0.9f;hp=80;dmg=12;sz=50;break;
        case ZOMBIE_RUNNER:  spd=2.2f;hp=40;dmg=8;sz=40;break;
        case ZOMBIE_TANK:    spd=0.5f;hp=250;dmg=25;sz=70;break;
        case ZOMBIE_WITCH:   spd=0.7f;hp=60;dmg=35;sz=45;break;
        case ZOMBIE_SPITTER: spd=0.8f;hp=70;dmg=15;sz=50;break;
        case ZOMBIE_LUNGER:  spd=1.4f;hp=55;dmg=20;sz=42;break;
        default:             spd=1.0f;hp=80;dmg=12;sz=50;break;
        }
        if(gameMode==0){spd*=0.7f;hp*=0.8f;dmg*=0.6f;}
        if(gameMode==2){spd*=1.3f;hp*=1.2f;dmg*=1.4f;}
        spd+=score*0.001f; hp+=killCount*0.3f;

        Enemy* e=new Enemy(sz,sz,spd,filename2,hp,dmg,type);
        float ex,ey; int side=rand()%4;
        if(side==0){ex=(float)(rand()%W);ey=-60;}
        else if(side==1){ex=(float)(rand()%W);ey=(float)(H+60);}
        else if(side==2){ex=-60;ey=(float)(rand()%H);}
        else{ex=(float)(W+60);ey=(float)(rand()%H);}
        e->setPositionX(ex);e->setPositionY(ey);
        enemies.push_back(e);
    }
}

void Game::SpawnBoss(){
    int W=glutGet(GLUT_WINDOW_WIDTH); if(W<100)return;
    boss=new Boss((float)(W/2-65),80,filename2,gameMode);
    bossAlive=true;bossMaxHealth=boss->getHealth();waveFlashTimer=120;
}

void Game::updateMovement(bool keys[256]){
    if(keys['w']||keys['W'])player->moveUP();
    if(keys['s']||keys['S'])player->moveDown();
    if(keys['a']||keys['A'])player->moveLeft();
    if(keys['d']||keys['D'])player->moveRight();
    clampPlayer();
}
void Game::clampPlayer(){
    int W=glutGet(GLUT_WINDOW_WIDTH),H=glutGet(GLUT_WINDOW_HEIGHT);
    if(player->getPositionX()<0)player->setPositionX(0);
    if(player->getPositionY()<0)player->setPositionY(0);
    if(player->getPositionX()+player->getWidth()>W)player->setPositionX((float)(W-player->getWidth()));
    if(player->getPositionY()+player->getHeight()>H)player->setPositionY((float)(H-player->getHeight()));
}

void Game::moveEnemy(Enemy* e){
    float dx=player->getPositionX()-e->getPositionX(),dy=player->getPositionY()-e->getPositionY();
    float len=sqrtf(dx*dx+dy*dy); if(len>0.1f){dx/=len;dy/=len;}
    float spd=e->getMoveSpeed();
    // Special abilities
    e->specialTimer++;
    if(e->zombieType==ZOMBIE_WITCH && e->specialCooldown>0 && e->specialTimer>=e->specialCooldown && len<300){
        e->specialActive=true; e->specialTimer=0;
    }
    if(e->zombieType==ZOMBIE_WITCH && e->specialActive && e->specialTimer>40) e->specialActive=false;

    if(e->zombieType==ZOMBIE_LUNGER && e->specialCooldown>0 && e->specialTimer>=e->specialCooldown && len<400 && len>100){
        e->specialActive=true; e->specialTimer=0;
        e->targetX=player->getPositionX(); e->targetY=player->getPositionY();
    }
    if(e->zombieType==ZOMBIE_LUNGER && e->specialActive){
        float ldx=e->targetX-e->getPositionX(), ldy=e->targetY-e->getPositionY();
        float ll=sqrtf(ldx*ldx+ldy*ldy);
        if(ll>10){dx=ldx/ll;dy=ldy/ll;} else e->specialActive=false;
    }

    if(Boss* b=dynamic_cast<Boss*>(e)){b->updateCharge(player->getPositionX(),player->getPositionY());spd=b->getEffectiveSpeed();}
    e->setPositionX(e->getPositionX()+dx*spd);
    e->setPositionY(e->getPositionY()+dy*spd);
    e->setAngle(atan2f(dy,dx)*180.0f/3.14159f);
}

void Game::onKeyPressed(unsigned char key,int x,int y){if(key==27)glutLeaveMainLoop();}
void Game::fireBullet(float mx,float my){
    float px=player->getPositionX()+player->getWidth()*0.5f,py=player->getPositionY()+player->getHeight()*0.5f;
    float ang=atan2f(my-py,mx-px);
    if(activeWeapon==2){for(int i=-2;i<=2;i++)bullets.push_back(new Bullet(px,py,ang+i*0.18f,BULLET_FLAME));director->reportShotFired();}
    else if(activeWeapon==1){float p1=-sinf(ang)*5,p2=cosf(ang)*5;bullets.push_back(new Bullet(px+p1,py+p2,ang,BULLET_RAPID));bullets.push_back(new Bullet(px-p1,py-p2,ang,BULLET_RAPID));director->reportShotFired();}
    else {bullets.push_back(new Bullet(px,py,ang,BULLET_PISTOL));director->reportShotFired();}
}
void Game::onMouseClicked(int b,int s,int x,int y){if(b==GLUT_LEFT_BUTTON&&s==GLUT_DOWN)fireBullet((float)x,(float)y);}
void Game::onMouseMove(int x,int y){
    float px=player->getPositionX()+player->getWidth()*0.5f,py=player->getPositionY()+player->getHeight()*0.5f;
    player->setAngle(atan2f((float)y-py,(float)x-px)*180.0f/3.14159f);
}

void Game::dropPerk(float x,float y){PerkType t[]={PERK_HEALTH,PERK_SHIELD,PERK_BETTER_GUN,PERK_FLAMETHROWER};perks.push_back(new Perk(x,y,t[rand()%4]));}
void Game::tryPickupPerks(){
    float px=player->getPositionX(),py=player->getPositionY(),pw=player->getWidth(),ph=player->getHeight();
    for(auto it=perks.begin();it!=perks.end();){
        Perk* p=*it;
        if(px<p->x+60&&px+pw>p->x&&py<p->y+60&&py+ph>p->y){
            float maxHp=(gameMode==0)?140:(gameMode==2)?70:100;
            switch(p->type){
            case PERK_HEALTH:{float h=player->getHealth()+40;if(h>maxHp)h=maxHp;player->setHealth(h);break;}
            case PERK_SHIELD:shieldActive=true;shieldTimer=1500;break;
            case PERK_BETTER_GUN:activeWeapon=1;weaponTimer=2500;break;
            case PERK_FLAMETHROWER:activeWeapon=2;weaponTimer=2500;break;
            }
            delete p;it=perks.erase(it);
        }else ++it;
    }
}

bool Game::detectCollision(Entity* a,Entity* b){
    if(!a||!b)return false;
    // Circle collision
    float cx_a = a->getPositionX() + a->getWidth() * 0.5f;
    float cy_a = a->getPositionY() + a->getHeight() * 0.5f;
    float cx_b = b->getPositionX() + b->getWidth() * 0.5f;
    float cy_b = b->getPositionY() + b->getHeight() * 0.5f;
    float dx = cx_a - cx_b;
    float dy = cy_a - cy_b;
    float distSq = dx * dx + dy * dy;
    float rad_a = a->getWidth() * 0.45f; // Slightly smaller radius for leniency
    float rad_b = b->getWidth() * 0.45f;
    float radiiSum = rad_a + rad_b;
    return distSq < (radiiSum * radiiSum);
}
void Game::pushBack(Entity* e1,Entity* e2){
    float cx1 = e1->getPositionX() + e1->getWidth() * 0.5f;
    float cy1 = e1->getPositionY() + e1->getHeight() * 0.5f;
    float cx2 = e2->getPositionX() + e2->getWidth() * 0.5f;
    float cy2 = e2->getPositionY() + e2->getHeight() * 0.5f;
    float dx = cx2 - cx1;
    float dy = cy2 - cy1;
    float len = sqrtf(dx * dx + dy * dy);
    if(len < 0.01f){ dx = 1; dy = 0; len = 1; }
    
    // Push e2 away from e1 based on overlap
    float rad1 = e1->getWidth() * 0.45f;
    float rad2 = e2->getWidth() * 0.45f;
    float overlap = (rad1 + rad2) - len;
    if (overlap > 0) {
        e2->setPositionX(e2->getPositionX() + (dx / len) * overlap * 0.5f);
        e2->setPositionY(e2->getPositionY() + (dy / len) * overlap * 0.5f);
        e1->setPositionX(e1->getPositionX() - (dx / len) * overlap * 0.5f);
        e1->setPositionY(e1->getPositionY() - (dy / len) * overlap * 0.5f);
    }
}

void Game::timer(void(*t)(int)){
    bgAnimTimer+=0.025f;
    if(weaponTimer>0){weaponTimer--;if(!weaponTimer)activeWeapon=0;}
    if(shieldTimer>0){shieldTimer--;if(!shieldTimer)shieldActive=false;}
    if(waveFlashTimer>0)waveFlashTimer--;
    if(damageFlashTimer>0)damageFlashTimer--;

    // Update AI Director
    if(!bossAlive) {
        director->update(this, player, enemies);
    }

    for(auto it=perks.begin();it!=perks.end();){(*it)->update();if((*it)->isExpired()){delete *it;it=perks.erase(it);}else ++it;}

    int W=glutGet(GLUT_WINDOW_WIDTH),H=glutGet(GLUT_WINDOW_HEIGHT);
    for(auto it=bullets.begin();it!=bullets.end();){
        (*it)->moveBullet();float bx=(*it)->getPositionX(),by=(*it)->getPositionY();
        if(bx<-50||bx>W+50||by<-50||by>H+50){delete *it;it=bullets.erase(it);}else ++it;
    }

    if(killCount>=nextBossKillThreshold&&!bossAlive){for(auto e:enemies)delete e;enemies.clear();SpawnBoss();nextBossKillThreshold+=15;}

    if(bossAlive&&boss){
        moveEnemy(boss);
        for(auto bIt=bullets.begin();bIt!=bullets.end();){
            if(detectCollision(boss,*bIt)){
                boss->setHealth(boss->getHealth()-(*bIt)->getDamage());delete *bIt;bIt=bullets.erase(bIt);
                if(boss->getHealth()<=0){dropPerk(boss->getPositionX(),boss->getPositionY());dropPerk(boss->getPositionX()+30,boss->getPositionY());
                    score+=200;killCount++;delete boss;boss=nullptr;bossAlive=false;waveFlashTimer=180;break;}
            }else ++bIt;
        }
        if(bossAlive&&boss&&detectCollision(player,boss)){
            if(shieldActive){shieldActive=false;shieldTimer=0;}
            else{player->setHealth(player->getHealth()-boss->getDamage()*0.06f);damageFlashTimer=10;pushBack(boss,player);if(player->getHealth()<=0)exit(0);}
        }
    }

    for(auto eIt=enemies.begin();eIt!=enemies.end();){
        moveEnemy(*eIt);bool died=false;
        for(auto bIt=bullets.begin();bIt!=bullets.end();){
            if(detectCollision(*eIt,*bIt)){
                (*eIt)->setHealth((*eIt)->getHealth()-(*bIt)->getDamage());delete *bIt;bIt=bullets.erase(bIt);
                if((*eIt)->getHealth()<=0){
                    float ex=(*eIt)->getPositionX(),ey=(*eIt)->getPositionY();
                    int pts=10;
                    if((*eIt)->zombieType==ZOMBIE_TANK)pts=30;
                    if((*eIt)->zombieType==ZOMBIE_WITCH)pts=25;
                    if((*eIt)->zombieType==ZOMBIE_LUNGER)pts=20;
                    delete *eIt;eIt=enemies.erase(eIt);died=true;score+=pts;killCount++;perkDropCounter++;
                    if(perkDropCounter>=3){dropPerk(ex,ey);perkDropCounter=0;} // drop every 3 kills
                    break;
                }
            }else ++bIt;
        }
        if(!died){
            if(detectCollision(player,*eIt)){
                if(shieldActive){shieldActive=false;shieldTimer=0;}
                else{player->setHealth(player->getHealth()-(*eIt)->getDamage()*0.03f);damageFlashTimer=6;pushBack(*eIt,player);if(player->getHealth()<=0)exit(0);}
            }
            
            // Check collision against other enemies (flocking separation)
            for(auto otherIt = enemies.begin(); otherIt != enemies.end(); ++otherIt) {
                if (*eIt != *otherIt && detectCollision(*eIt, *otherIt)) {
                    pushBack(*otherIt, *eIt);
                }
            }
            
            ++eIt;
        }
    }
    tryPickupPerks();glutPostRedisplay();glutTimerFunc(10,t,0);
}

// Drawing
static void fillRect(float x,float y,float w,float h){glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();}
static void outlineRect(float x,float y,float w,float h){glBegin(GL_LINE_LOOP);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();}
static void renderText(float x,float y,void* f,const char* s){glRasterPos2f(x,y);for(const char* c=s;*c;c++)glutBitmapCharacter(f,(int)*c);}
static void renderTextC(float cx,float y,void* f,const char* s){int w=0;for(const char* c=s;*c;c++)w+=glutBitmapWidth(f,(int)*c);renderText(cx-(float)w/2,y,f,s);}

void Game::drawBackground(){
    int W=glutGet(GLUT_WINDOW_WIDTH),H=glutGet(GLUT_WINDOW_HEIGHT);
    if(damageFlashTimer>0)glColor3f(0.35f,0.0f,0.0f);else glColor3f(0.07f,0.09f,0.12f);
    fillRect(0,0,(float)W,(float)H);
    glColor3f(0.12f,0.15f,0.19f);glLineWidth(1.0f);glBegin(GL_LINES);
    for(int gx=0;gx<=W;gx+=64){glVertex2f((float)gx,0);glVertex2f((float)gx,(float)H);}
    for(int gy=0;gy<=H;gy+=64){glVertex2f(0,(float)gy);glVertex2f((float)W,(float)gy);}
    glEnd();
    srand(7777);glColor3f(0.22f,0.05f,0.05f);glPointSize(5.0f);glBegin(GL_POINTS);
    for(int i=0;i<80;i++)glVertex2f((float)(rand()%W),(float)(rand()%H));
    glEnd();glPointSize(1.0f);srand((unsigned)time(nullptr));
}

void Game::drawEnemyHealthBars(){
    for(auto e:enemies){
        float ex=e->getPositionX(),ey=e->getPositionY(),ew=e->getWidth();
        float pct=e->getHealth()/e->getMaxHealth();if(pct>1)pct=1;if(pct<0)pct=0;
        float bw=ew,bh=4,bx=ex,by=ey-8;
        glColor3f(0.3f,0.0f,0.0f);fillRect(bx,by,bw,bh);
        float r=(pct<0.5f)?1:2*(1-pct),g=(pct>0.5f)?1:2*pct;
        glColor3f(r,g,0);fillRect(bx,by,bw*pct,bh);
        glColor3f(0.5f,0.5f,0.5f);outlineRect(bx,by,bw,bh);
        // Type label
        const char* lbl="";
        switch(e->zombieType){case ZOMBIE_RUNNER:lbl="RUNNER";break;case ZOMBIE_TANK:lbl="TANK";break;
        case ZOMBIE_WITCH:lbl="WITCH";break;case ZOMBIE_SPITTER:lbl="SPITTER";break;case ZOMBIE_LUNGER:lbl="LUNGER";break;default:break;}
        if(lbl[0]){glColor3f(0.7f,0.7f,0.7f);glRasterPos2f(ex,ey-12);for(const char*c=lbl;*c;c++)glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10,(int)*c);}
    }
}

void Game::drawHUD(){
    int W=glutGet(GLUT_WINDOW_WIDTH),H=glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);glPushMatrix();glLoadIdentity();glOrtho(0,W,H,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glPushMatrix();glLoadIdentity();
    char buf[80];
    
    // Modern L4D2 Style HUD at the bottom left
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Sleek translucent background panel
    glColor4f(0.05f,0.05f,0.05f,0.7f);
    fillRect(20, H - 120, 320, 100);
    
    // Subtle border
    glColor4f(0.4f,0.4f,0.4f,0.5f);
    glLineWidth(2.0f);
    outlineRect(20, H - 120, 320, 100);
    glLineWidth(1.0f);

    // Score & Kills
    glColor4f(0.9f,0.9f,0.9f, 1.0f);
    sprintf(buf,"SCORE: %d",score);
    renderText(35, H - 95, GLUT_BITMAP_HELVETICA_18, buf);
    
    glColor4f(0.6f,0.6f,0.6f, 1.0f);
    sprintf(buf,"KILLS: %d  |  ALIVE: %d",killCount,(int)enemies.size()+(bossAlive?1:0));
    renderText(35, H - 75, GLUT_BITMAP_HELVETICA_12, buf);
    
    // Modern Healthbar
    float maxHp=(gameMode==0)?140:(gameMode==2)?70:100;
    float hpPct=player->getHealth()/maxHp;
    hpPct=hpPct<0?0:(hpPct>1?1:hpPct);
    float bw=280;
    
    glColor4f(0.2f,0.0f,0.0f, 0.8f); // Dark red background
    fillRect(35, H - 60, bw, 18);
    
    // Dynamic color (Green -> Yellow -> Red)
    float rColor = (hpPct > 0.5f) ? (1.0f - (hpPct - 0.5f) * 2.0f) : 1.0f;
    float gColor = (hpPct > 0.5f) ? 1.0f : (hpPct * 2.0f);
    float bColor = 0.1f;
    
    glColor4f(rColor, gColor, bColor, 0.9f);
    fillRect(35, H - 60, bw * hpPct, 18);
    
    // Glossy overlay for healthbar
    glColor4f(1.0f, 1.0f, 1.0f, 0.2f);
    fillRect(35, H - 60, bw * hpPct, 9);
    
    glColor4f(0.8f,0.8f,0.8f, 1.0f);
    outlineRect(35, H - 60, bw, 18);
    
    // Weapon info
    const char*wn="PISTOL";float wr=0.8f,wg=0.8f,wb=0.8f;
    if(activeWeapon==1){wn="SMG (RAPID)";wr=0.4f;wg=0.8f;wb=1.0f;}
    if(activeWeapon==2){wn="FLAMETHROWER";wr=1.0f;wg=0.5f;wb=0.1f;}
    glColor4f(wr,wg,wb, 1.0f);
    sprintf(buf,"WEAPON: %s",wn);
    renderText(35, H - 30, GLUT_BITMAP_HELVETICA_12, buf);

    if(shieldActive){
        glColor4f(0.2f,0.6f,1.0f, 1.0f);
        renderText(180, H - 30, GLUT_BITMAP_HELVETICA_12,"SHIELD ACTIVE");
    }

    // Stress Level Indicator (AI Director)
    float stress = director->getStressLevel();
    glColor4f(0.8f, 0.2f, 0.2f, 0.8f);
    sprintf(buf,"DIRECTOR STRESS: %d%%", (int)(stress * 100));
    renderText(20, 25, GLUT_BITMAP_HELVETICA_10, buf);

    // Wave/Boss flash
    if(waveFlashTimer>0){
        float a=(float)waveFlashTimer/120.0f;
        glColor4f(1.0f, 0.2f, 0.2f, a);
        renderTextC((float)(W/2),(float)(H/2)-20,GLUT_BITMAP_TIMES_ROMAN_24,bossAlive?"!! BOSS INCOMING !!":"BOSS DEFEATED!");
    }
    
    // Damage vignette
    if(damageFlashTimer>0){
        float a=(float)damageFlashTimer/10.0f * 0.5f;
        glColor4f(1.0f, 0.0f, 0.0f, a);
        fillRect(0,0,W,H); // Red flash over screen
    }

    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION);glPopMatrix();glMatrixMode(GL_MODELVIEW);glPopMatrix();
}

void Game::draw(){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    int W=glutGet(GLUT_WINDOW_WIDTH),H=glutGet(GLUT_WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,W,H,0,-1,1);
    glMatrixMode(GL_MODELVIEW);glLoadIdentity();
    drawBackground();
    for(auto p:perks)p->draw();
    for(auto e:enemies)e->draw();
    if(bossAlive&&boss)boss->draw();
    drawEnemyHealthBars();
    for(auto b:bullets)b->draw();
    player->draw();
    
    // Dynamic Flashlight Overlay
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float px = player->getPositionX() + player->getWidth() * 0.5f;
    float py = player->getPositionY() + player->getHeight() * 0.5f;
    
    int numSegments = 72;
    float outerRadius = 3000.0f;
    float fov = 65.0f;
    float pAngle = player->getAngle();

    while(pAngle < 0) pAngle += 360.0f;
    while(pAngle >= 360) pAngle -= 360.0f;
    
    auto getLightRadius = [&](float a) {
        float diff = fmod(abs(a - pAngle), 360.0f);
        if (diff > 180.0f) diff = 360.0f - diff;
        
        if (diff < fov / 2.0f) {
            return 700.0f; 
        } else if (diff < fov / 2.0f + 20.0f) {
            float t = (diff - fov / 2.0f) / 20.0f;
            return 700.0f * (1.0f - t) + 120.0f * t;
        } else {
            return 120.0f;
        }
    };

    // 1. Draw outer solid darkness
    glBegin(GL_QUADS);
    for (int i = 0; i < numSegments; ++i) {
        float angle1 = (i * 360.0f / numSegments);
        float angle2 = ((i + 1) * 360.0f / numSegments);
        
        float r1 = getLightRadius(angle1);
        float r2 = getLightRadius(angle2);
        
        float a1_rad = angle1 * 3.14159f / 180.0f;
        float a2_rad = angle2 * 3.14159f / 180.0f;

        glColor4f(0.01f, 0.01f, 0.02f, 0.96f);
        glVertex2f(px + r1 * cosf(a1_rad), py + r1 * sinf(a1_rad));
        glVertex2f(px + r2 * cosf(a2_rad), py + r2 * sinf(a2_rad));
        glVertex2f(px + outerRadius * cosf(a2_rad), py + outerRadius * sinf(a2_rad));
        glVertex2f(px + outerRadius * cosf(a1_rad), py + outerRadius * sinf(a1_rad));
    }
    glEnd();

    // 2. Draw soft inner gradient (light to dark)
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < numSegments; ++i) {
        float angle1 = (i * 360.0f / numSegments);
        float angle2 = ((i + 1) * 360.0f / numSegments);
        
        float r1 = getLightRadius(angle1);
        float r2 = getLightRadius(angle2);
        
        float a1_rad = angle1 * 3.14159f / 180.0f;
        float a2_rad = angle2 * 3.14159f / 180.0f;

        // Center is fully transparent
        glColor4f(0.01f, 0.01f, 0.02f, 0.0f);
        glVertex2f(px, py);
        
        // Edge is fully dark
        glColor4f(0.01f, 0.01f, 0.02f, 0.96f);
        glVertex2f(px + r1 * cosf(a1_rad), py + r1 * sinf(a1_rad));
        glVertex2f(px + r2 * cosf(a2_rad), py + r2 * sinf(a2_rad));
    }
    glEnd();

    glDisable(GL_BLEND);

    drawHUD();
    glutSwapBuffers();
}
#include "Game.h"
#include <ctime>
#include <cstring>
#include <cmath>
#include <cstdio>

Game::Game() {
    srand((unsigned)time(nullptr));
    filename=strdup("res/player.bmp"); filename2=strdup("res/enemy.bmp"); filename3=strdup("res/bullet.bmp");
    player=new Player(60,60,5,filename,100); boss=nullptr;
    score=0;killCount=0;gameMode=1;spawnTimer=0;activeWeapon=0;weaponTimer=0;
    shieldActive=false;shieldTimer=0;bossAlive=false;bossMaxHealth=800;
    nextBossKillThreshold=15;bgAnimTimer=0;perkDropCounter=0;waveFlashTimer=0;damageFlashTimer=0;
    player->setPositionX(400);player->setPositionY(300);
}
Game::~Game(){delete player;if(boss)delete boss;for(auto e:enemies)delete e;for(auto b:bullets)delete b;for(auto p:perks)delete p;}

void Game::reset(){
    score=0;killCount=0;spawnTimer=0;activeWeapon=0;weaponTimer=0;shieldActive=false;shieldTimer=0;
    bossAlive=false;bossMaxHealth=800;nextBossKillThreshold=15;bgAnimTimer=0;perkDropCounter=0;waveFlashTimer=0;damageFlashTimer=0;
    if(boss){delete boss;boss=nullptr;}
    for(auto e:enemies)delete e;enemies.clear();for(auto b:bullets)delete b;bullets.clear();for(auto p:perks)delete p;perks.clear();
    float hp=(gameMode==0)?140:(gameMode==2)?70:100;
    player->setHealth(hp);player->setPositionX(400);player->setPositionY(300);
}
void Game::setGameMode(unsigned int m){gameMode=m;reset();}

void Game::SpawnEnemy(){
    int W=glutGet(GLUT_WINDOW_WIDTH),H=glutGet(GLUT_WINDOW_HEIGHT);
    if(W<100||H<100)return;
    // Pick zombie type with weighted random
    ZombieType type=ZOMBIE_WALKER;
    int roll=rand()%100;
    if(roll<35) type=ZOMBIE_WALKER;
    else if(roll<55) type=ZOMBIE_RUNNER;
    else if(roll<70) type=ZOMBIE_TANK;
    else if(roll<80) type=ZOMBIE_WITCH;
    else if(roll<90) type=ZOMBIE_SPITTER;
    else type=ZOMBIE_LUNGER;

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

    int count=1+(killCount/40); if(count>3)count=3;
    for(int i=0;i<count;i++){
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
    if(activeWeapon==2){for(int i=-2;i<=2;i++)bullets.push_back(new Bullet(px,py,ang+i*0.18f,BULLET_FLAME));}
    else if(activeWeapon==1){float p1=-sinf(ang)*5,p2=cosf(ang)*5;bullets.push_back(new Bullet(px+p1,py+p2,ang,BULLET_RAPID));bullets.push_back(new Bullet(px-p1,py-p2,ang,BULLET_RAPID));}
    else bullets.push_back(new Bullet(px,py,ang,BULLET_PISTOL));
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
    return(a->getPositionX()+a->getWidth()>=b->getPositionX()&&b->getPositionX()+b->getWidth()>=a->getPositionX()&&
           a->getPositionY()+a->getHeight()>=b->getPositionY()&&b->getPositionY()+b->getHeight()>=a->getPositionY());
}
void Game::pushBack(Entity* e1,Entity* e2){
    float dx=e2->getPositionX()-e1->getPositionX(),dy=e2->getPositionY()-e1->getPositionY();
    float len=sqrtf(dx*dx+dy*dy);if(len<0.01f){dx=1;dy=0;len=1;}
    e2->setPositionX(e2->getPositionX()+(dx/len)*30);e2->setPositionY(e2->getPositionY()+(dy/len)*30);
}

void Game::timer(void(*t)(int)){
    bgAnimTimer+=0.025f;
    if(weaponTimer>0){weaponTimer--;if(!weaponTimer)activeWeapon=0;}
    if(shieldTimer>0){shieldTimer--;if(!shieldTimer)shieldActive=false;}
    if(waveFlashTimer>0)waveFlashTimer--;
    if(damageFlashTimer>0)damageFlashTimer--;

    spawnTimer++;
    int spawnRate=(gameMode==0)?130:(gameMode==2)?60:85;
    if(spawnTimer>=spawnRate&&!bossAlive){SpawnEnemy();spawnTimer=0;}
    if(killCount==0&&enemies.empty()&&!bossAlive&&spawnTimer<2)SpawnEnemy();

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
            }++eIt;
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
    // Panel
    glColor3f(0.06f,0.08f,0.11f);fillRect(8,8,280,95);
    glColor3f(0.25f,0.35f,0.50f);glLineWidth(2.0f);outlineRect(8,8,280,95);glLineWidth(1.0f);
    // Score
    glColor3f(1.0f,0.85f,0.15f);sprintf(buf,"SCORE  %d",score);renderText(18,26,GLUT_BITMAP_HELVETICA_18,buf);
    // Kills + enemies
    glColor3f(0.75f,0.75f,0.75f);sprintf(buf,"KILLS %d  |  ALIVE %d",killCount,(int)enemies.size()+(bossAlive?1:0));
    renderText(18,44,GLUT_BITMAP_HELVETICA_12,buf);
    // HP bar
    float maxHp=(gameMode==0)?140:(gameMode==2)?70:100;float hpPct=player->getHealth()/maxHp;
    hpPct=hpPct<0?0:(hpPct>1?1:hpPct);float bw=220;
    glColor3f(0.5f,0.5f,0.5f);renderText(18,62,GLUT_BITMAP_HELVETICA_12,"HP");
    glColor3f(0.25f,0.0f,0.0f);fillRect(38,53,bw,12);
    float r2=hpPct<0.5f?1:2*(1-hpPct),g2=hpPct>0.5f?1:2*hpPct;
    glColor3f(r2,g2,0);fillRect(38,53,bw*hpPct,12);glColor3f(0.6f,0.6f,0.6f);outlineRect(38,53,bw,12);
    // Weapon
    const char*wn="PISTOL";float wr=1,wg=1,wb=0.2f;
    if(activeWeapon==1){wn="RAPID FIRE";wr=0.1f;wg=0.95f;wb=1.0f;}
    if(activeWeapon==2){wn="FLAMETHROWER";wr=1.0f;wg=0.4f;wb=0.0f;}
    glColor3f(wr,wg,wb);sprintf(buf,"GUN: %s",wn);renderText(18,82,GLUT_BITMAP_HELVETICA_10,buf);
    if(shieldActive){glColor3f(0.3f,0.7f,1.0f);renderText(140,82,GLUT_BITMAP_HELVETICA_10,"SHIELD");}
    // Wave flash
    if(waveFlashTimer>0){float a=(float)waveFlashTimer/120.0f;glColor3f(a,0.2f*a,0.2f*a);
        renderTextC((float)(W/2),(float)(H/2)-20,GLUT_BITMAP_TIMES_ROMAN_24,bossAlive?"!! BOSS INCOMING !!":"BOSS DEFEATED!");}
    // Boss warning
    int bossIn=nextBossKillThreshold-killCount;
    if(bossIn<=5&&bossIn>0&&!bossAlive){float p=0.6f+0.4f*sinf(bgAnimTimer*5);glColor3f(p,0.1f,0.1f);
        sprintf(buf,"BOSS IN %d KILLS",bossIn);renderTextC((float)(W/2),18,GLUT_BITMAP_HELVETICA_18,buf);}
    // Boss HP
    if(bossAlive&&boss){float bHp=boss->getHealth()/bossMaxHealth;if(bHp<0)bHp=0;float bBarW=400,bBarX=(W-bBarW)/2.0f;
        glColor3f(0.06f,0.02f,0.02f);fillRect(bBarX-2,6,bBarW+4,26);
        glColor3f(0.35f,0,0);fillRect(bBarX,8,bBarW,22);glColor3f(1-bHp*0.3f,0.1f,0.1f);
        fillRect(bBarX,8,bBarW*bHp,22);glColor3f(1,0.4f,0.4f);outlineRect(bBarX,8,bBarW,22);
        glColor3f(1,0.8f,0.8f);renderTextC(bBarX+bBarW/2,22,GLUT_BITMAP_HELVETICA_12,"BOSS");}
    // Controls
    glColor3f(0.3f,0.38f,0.48f);renderText((float)(W-260),(float)(H-10),GLUT_BITMAP_HELVETICA_10,"WASD:Move  LMB:Shoot  ESC:Menu");
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
    drawHUD();
    glutSwapBuffers();
}
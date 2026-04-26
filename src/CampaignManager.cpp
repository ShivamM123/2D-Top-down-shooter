#include "CampaignManager.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

// ── Helper drawing ──
static void cmFillRect(float x,float y,float w,float h){
    glBegin(GL_QUADS);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();
}
static void cmOutlineRect(float x,float y,float w,float h){
    glBegin(GL_LINE_LOOP);glVertex2f(x,y);glVertex2f(x+w,y);glVertex2f(x+w,y+h);glVertex2f(x,y+h);glEnd();
}
static void cmCircle(float cx, float cy, float r, int n){
    glBegin(GL_POLYGON);
    for(int i=0;i<n;i++){float a=i*6.2832f/n;glVertex2f(cx+r*cosf(a),cy+r*sinf(a));}
    glEnd();
}
static void cmCircleOutline(float cx, float cy, float r, int n){
    glBegin(GL_LINE_LOOP);
    for(int i=0;i<n;i++){float a=i*6.2832f/n;glVertex2f(cx+r*cosf(a),cy+r*sinf(a));}
    glEnd();
}
static void cmText(float x, float y, void* font, const char* s){
    glRasterPos2f(x,y);for(const char*c=s;*c;c++)glutBitmapCharacter(font,(int)*c);
}
static void cmTextCenter(float cx, float y, void* font, const char* s){
    int w=0;for(const char*c=s;*c;c++)w+=glutBitmapWidth(font,(int)*c);
    cmText(cx-(float)w/2,y,font,s);
}

// ═══════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════
CampaignManager::CampaignManager()
    : currentAct(ACT_SCAVENGE),
      actTimer(0), totalFrames(0),
      fuelsCollected(0), fuelsRequired(3),
      holdoutDuration(3600), holdoutTimer(0),
      generatorActive(false),
      hordeSpawnTimer(0), hordeSpawnInterval(80),
      playerReachedExtraction(false),
      dialogCharIndex(0), dialogTimer(0),
      dialogDuration(400), dialogActive(false),
      typewriterSpeed(3), pulseTimer(0)
{
    extractionZone = Waypoint(0, 0, "EXTRACTION");
}

// ═══════════════════════════════════════════════════════════════
// Initialize waypoints — called once when screen size is known
// ═══════════════════════════════════════════════════════════════
void CampaignManager::initWaypoints(int W, int H) {
    fuelCans.clear();
    fuelsCollected = 0;

    // Place 3 fuel cans at spread-out locations
    fuelCans.push_back(Waypoint((float)(W * 0.15f), (float)(H * 0.25f), "FUEL CAN A"));
    fuelCans.push_back(Waypoint((float)(W * 0.80f), (float)(H * 0.20f), "FUEL CAN B"));
    fuelCans.push_back(Waypoint((float)(W * 0.50f), (float)(H * 0.75f), "FUEL CAN C"));

    // Generator location (for Act 2) — center of screen
    // Extraction zone (for Act 3) — top right
    extractionZone = Waypoint((float)(W * 0.90f), (float)(H * 0.10f), "HELICOPTER");
    extractionZone.radius = 60;

    showRadioMessage("Command: Survivor, we need you to locate 3 fuel cans. Move to the markers on your HUD.", 600);
}

// ═══════════════════════════════════════════════════════════════
// Core Update
// ═══════════════════════════════════════════════════════════════
int CampaignManager::update(float px, float py, float pw, float ph,
                            int screenW, int screenH) {
    totalFrames++;
    actTimer++;
    pulseTimer += 0.08f;
    if(pulseTimer > 6.28f) pulseTimer -= 6.28f;

    int enemiesToSpawn = 0;

    // Update dialog typewriter
    if (dialogActive) {
        dialogTimer++;
        if (dialogTimer % typewriterSpeed == 0 && dialogCharIndex < (int)currentDialogText.size()) {
            dialogCharIndex++;
            displayedDialogText = currentDialogText.substr(0, dialogCharIndex);
        }
        if (dialogTimer >= dialogDuration) {
            dialogActive = false;
        }
    }

    float pcx = px + pw/2;
    float pcy = py + ph/2;

    switch (currentAct) {
    // ─────────────────────────────────────────────
    // ACT 1: SCAVENGE — collect fuel cans
    // ─────────────────────────────────────────────
    case ACT_SCAVENGE: {
        for (auto& wp : fuelCans) {
            if (wp.collected) continue;
            float dx = pcx - wp.x;
            float dy = pcy - wp.y;
            float dist = sqrtf(dx*dx + dy*dy);
            if (dist < wp.radius + 30) {
                wp.collected = true;
                fuelsCollected++;

                if (fuelsCollected == 1)
                    showRadioMessage("Command: Good, one down. Keep moving, we're tracking hostiles nearby.", 400);
                else if (fuelsCollected == 2)
                    showRadioMessage("Command: Two cans secured. One more to go. Watch your six.", 400);
                else if (fuelsCollected >= fuelsRequired) {
                    showRadioMessage("Command: All fuel collected! Head to the GENERATOR at center screen. Brace yourself.", 500);
                    // Transition to ACT 2
                    currentAct = ACT_HOLDOUT;
                    actTimer = 0;
                    holdoutTimer = 0;
                    generatorActive = false;
                }
            }
        }
        // Light ambient spawning during scavenge
        if (actTimer % 200 == 0 && actTimer > 100) {
            enemiesToSpawn = 2;
        }
        break;
    }

    // ─────────────────────────────────────────────
    // ACT 2: HOLDOUT — survive the horde
    // ─────────────────────────────────────────────
    case ACT_HOLDOUT: {
        // Generator is at center screen
        float genX = screenW * 0.5f;
        float genY = screenH * 0.5f;

        if (!generatorActive) {
            // Player must reach the generator to activate it
            float dx = pcx - genX;
            float dy = pcy - genY;
            float dist = sqrtf(dx*dx + dy*dy);
            if (dist < 80) {
                generatorActive = true;
                holdoutTimer = 0;
                showRadioMessage("Pilot: Generator is online! Hold your position for 2 MINUTES! They're coming!", 500);
            }
        }
        else {
            // Horde is active!
            holdoutTimer++;
            hordeSpawnTimer++;

            // Spawn intensity ramps up over time
            int currentInterval = hordeSpawnInterval;
            float progress = (float)holdoutTimer / (float)holdoutDuration;
            if (progress > 0.7f) currentInterval = 30;       // Final 30%: insane
            else if (progress > 0.4f) currentInterval = 50;   // Mid: heavy
            else currentInterval = 80;                         // Start: moderate

            if (hordeSpawnTimer >= currentInterval) {
                int count = (progress > 0.7f) ? 4 : (progress > 0.4f) ? 3 : 2;
                enemiesToSpawn = count;
                hordeSpawnTimer = 0;
            }

            // Periodic radio updates
            if (holdoutTimer == 600)
                showRadioMessage("Pilot: 90 seconds remaining! Keep fighting!", 350);
            if (holdoutTimer == 1800)
                showRadioMessage("Pilot: One minute left! The chopper is inbound!", 350);
            if (holdoutTimer == 3000)
                showRadioMessage("Pilot: 30 seconds! GET READY TO MOVE!", 350);

            // Holdout complete
            if (holdoutTimer >= holdoutDuration) {
                currentAct = ACT_ESCAPE;
                actTimer = 0;
                showRadioMessage("Pilot: Generator charged! We're on the roof! GET TO THE EXTRACTION ZONE NOW!", 600);
            }
        }

        // Small ambient spawns even before generator activated
        if (!generatorActive && actTimer % 150 == 0) {
            enemiesToSpawn = 1;
        }
        break;
    }

    // ─────────────────────────────────────────────
    // ACT 3: ESCAPE — reach extraction
    // ─────────────────────────────────────────────
    case ACT_ESCAPE: {
        float dx = pcx - extractionZone.x;
        float dy = pcy - extractionZone.y;
        float dist = sqrtf(dx*dx + dy*dy);

        if (dist < extractionZone.radius + 30) {
            playerReachedExtraction = true;
            currentAct = ACT_VICTORY;
            actTimer = 0;
            showRadioMessage("Pilot: You made it! Pulling you out! MISSION COMPLETE!", 800);
        }

        // Desperate enemies chasing during escape
        if (actTimer % 60 == 0) {
            enemiesToSpawn = 3;
        }

        if (actTimer == 300)
            showRadioMessage("Pilot: We are dusting off in 60 seconds, GET TO THE ROOF!", 400);
        break;
    }

    case ACT_VICTORY:
        // No more spawns
        break;
    }

    return enemiesToSpawn;
}

// ═══════════════════════════════════════════════════════════════
// Show a radio message with typewriter effect
// ═══════════════════════════════════════════════════════════════
void CampaignManager::showRadioMessage(const std::string& text, int durationFrames) {
    currentDialogText = text;
    displayedDialogText = "";
    dialogCharIndex = 0;
    dialogTimer = 0;
    dialogDuration = durationFrames;
    dialogActive = true;
}

// ═══════════════════════════════════════════════════════════════
// Draw Waypoints (in world space, before HUD overlay)
// ═══════════════════════════════════════════════════════════════
void CampaignManager::drawWaypoints() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float pulse = 0.6f + 0.4f * sinf(pulseTimer);

    if (currentAct == ACT_SCAVENGE) {
        for (auto& wp : fuelCans) {
            if (wp.collected) continue;

            // Pulsing ring
            glColor4f(1.0f, 0.8f, 0.0f, 0.4f * pulse);
            cmCircle(wp.x, wp.y, wp.radius + 5, 24);

            // Inner marker
            glColor4f(1.0f, 0.9f, 0.2f, 0.8f);
            cmCircle(wp.x, wp.y, 12, 16);

            // Fuel can icon (simple rectangle)
            glColor4f(0.8f, 0.2f, 0.0f, 0.9f);
            cmFillRect(wp.x - 8, wp.y - 12, 16, 24);
            glColor4f(1.0f, 0.4f, 0.0f, 1.0f);
            cmFillRect(wp.x - 5, wp.y - 16, 10, 6); // handle

            // Ring outline
            glColor4f(1.0f, 0.8f, 0.0f, 0.7f);
            glLineWidth(2.0f);
            cmCircleOutline(wp.x, wp.y, wp.radius, 24);
            glLineWidth(1.0f);

            // Label
            glColor4f(1.0f, 0.9f, 0.3f, 1.0f);
            cmTextCenter(wp.x, wp.y - wp.radius - 8, GLUT_BITMAP_HELVETICA_10, wp.label);
        }
    }

    if (currentAct == ACT_HOLDOUT && !generatorActive) {
        int W = glutGet(GLUT_WINDOW_WIDTH), H = glutGet(GLUT_WINDOW_HEIGHT);
        float gx = W * 0.5f, gy = H * 0.5f;

        // Generator marker
        glColor4f(0.0f, 0.8f, 1.0f, 0.3f * pulse);
        cmCircle(gx, gy, 70, 32);
        glColor4f(0.0f, 0.8f, 1.0f, 0.8f);
        glLineWidth(2.5f);
        cmCircleOutline(gx, gy, 60, 32);
        glLineWidth(1.0f);

        // Generator icon (box with lightning bolt)
        glColor4f(0.2f, 0.6f, 0.9f, 0.9f);
        cmFillRect(gx - 15, gy - 20, 30, 40);
        glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
        glBegin(GL_TRIANGLES);
        glVertex2f(gx - 4, gy - 15);
        glVertex2f(gx + 6, gy);
        glVertex2f(gx - 2, gy);
        glVertex2f(gx + 4, gy);
        glVertex2f(gx - 6, gy + 15);
        glVertex2f(gx + 2, gy);
        glEnd();

        glColor4f(0.3f, 0.9f, 1.0f, 1.0f);
        cmTextCenter(gx, gy - 70, GLUT_BITMAP_HELVETICA_12, "ACTIVATE GENERATOR");
    }

    if (currentAct == ACT_ESCAPE) {
        // Extraction zone
        float ex = extractionZone.x, ey = extractionZone.y;

        // Pulsing green zone
        glColor4f(0.0f, 1.0f, 0.3f, 0.2f * pulse);
        cmCircle(ex, ey, extractionZone.radius + 15, 32);
        glColor4f(0.0f, 1.0f, 0.3f, 0.5f);
        cmCircle(ex, ey, extractionZone.radius, 32);

        // Helicopter pad (H)
        glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
        glLineWidth(3.0f);
        glBegin(GL_LINES);
        // H shape
        glVertex2f(ex - 15, ey - 18); glVertex2f(ex - 15, ey + 18);
        glVertex2f(ex + 15, ey - 18); glVertex2f(ex + 15, ey + 18);
        glVertex2f(ex - 15, ey);      glVertex2f(ex + 15, ey);
        glEnd();
        glLineWidth(1.0f);

        glColor4f(0.0f, 1.0f, 0.3f, 0.9f);
        glLineWidth(2.5f);
        cmCircleOutline(ex, ey, extractionZone.radius, 32);
        glLineWidth(1.0f);

        glColor4f(0.2f, 1.0f, 0.4f, 1.0f);
        cmTextCenter(ex, ey - extractionZone.radius - 10, GLUT_BITMAP_HELVETICA_12, "EXTRACTION ZONE");
    }

    glDisable(GL_BLEND);
}

// ═══════════════════════════════════════════════════════════════
// Draw Objective HUD (top of screen)
// ═══════════════════════════════════════════════════════════════
void CampaignManager::drawObjectives(int W, int H) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    char buf[128];

    // Objective banner (top center)
    float bannerW = 350, bannerH = 45;
    float bx = (W - bannerW) / 2.0f, by = 50;

    glColor4f(0.02f, 0.02f, 0.05f, 0.75f);
    cmFillRect(bx, by, bannerW, bannerH);
    glColor4f(0.5f, 0.5f, 0.5f, 0.4f);
    cmOutlineRect(bx, by, bannerW, bannerH);

    switch (currentAct) {
    case ACT_SCAVENGE:
        glColor4f(1.0f, 0.9f, 0.2f, 1.0f);
        cmTextCenter(W/2.0f, by + 18, GLUT_BITMAP_HELVETICA_12, "ACT 1: SCAVENGE");
        glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
        sprintf(buf, "Collect Fuel Cans: %d / %d", fuelsCollected, fuelsRequired);
        cmTextCenter(W/2.0f, by + 35, GLUT_BITMAP_HELVETICA_12, buf);
        break;

    case ACT_HOLDOUT:
        if (!generatorActive) {
            glColor4f(0.2f, 0.8f, 1.0f, 1.0f);
            cmTextCenter(W/2.0f, by + 18, GLUT_BITMAP_HELVETICA_12, "ACT 2: HOLDOUT");
            glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
            cmTextCenter(W/2.0f, by + 35, GLUT_BITMAP_HELVETICA_12, "Reach the GENERATOR to activate it");
        } else {
            // Show timer
            int framesLeft = holdoutDuration - holdoutTimer;
            int secondsLeft = framesLeft / 100; // ~100fps timer tick
            if (secondsLeft < 0) secondsLeft = 0;

            glColor4f(1.0f, 0.3f, 0.1f, 1.0f);
            cmTextCenter(W/2.0f, by + 18, GLUT_BITMAP_HELVETICA_12, "ACT 2: SURVIVE THE HORDE");

            // Timer with urgency color
            float urgency = (float)holdoutTimer / (float)holdoutDuration;
            if (urgency > 0.8f) glColor4f(0.2f, 1.0f, 0.2f, 1.0f); // Almost done = green
            else if (urgency > 0.5f) glColor4f(1.0f, 0.8f, 0.2f, 1.0f); // Yellow
            else glColor4f(1.0f, 0.3f, 0.3f, 1.0f); // Red

            sprintf(buf, "TIME REMAINING: %d:%02d", secondsLeft / 60, secondsLeft % 60);
            cmTextCenter(W/2.0f, by + 35, GLUT_BITMAP_HELVETICA_12, buf);

            // Progress bar
            glColor4f(0.2f, 0.2f, 0.2f, 0.8f);
            cmFillRect(bx + 20, by + bannerH + 5, bannerW - 40, 8);
            glColor4f(0.2f, 0.9f, 0.3f, 0.9f);
            cmFillRect(bx + 20, by + bannerH + 5, (bannerW - 40) * getHoldoutProgress(), 8);
        }
        break;

    case ACT_ESCAPE:
        glColor4f(0.0f, 1.0f, 0.3f, 1.0f);
        cmTextCenter(W/2.0f, by + 18, GLUT_BITMAP_HELVETICA_12, "ACT 3: ESCAPE");
        glColor4f(0.8f, 0.8f, 0.8f, 1.0f);
        cmTextCenter(W/2.0f, by + 35, GLUT_BITMAP_HELVETICA_12, "REACH THE EXTRACTION ZONE!");
        break;

    case ACT_VICTORY: {
        // Victory screen overlay
        glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
        cmFillRect(0, 0, (float)W, (float)H);

        glColor4f(0.1f, 0.8f, 0.2f, 1.0f);
        cmTextCenter(W/2.0f, H/2.0f - 40, GLUT_BITMAP_TIMES_ROMAN_24, "MISSION COMPLETE");

        glColor4f(0.9f, 0.9f, 0.9f, 1.0f);
        cmTextCenter(W/2.0f, H/2.0f, GLUT_BITMAP_HELVETICA_18, "You survived the outbreak.");

        glColor4f(0.6f, 0.6f, 0.6f, 1.0f);
        cmTextCenter(W/2.0f, H/2.0f + 30, GLUT_BITMAP_HELVETICA_12, "Press ESC to return to menu");
        break;
    }
    }

    glDisable(GL_BLEND);
}

// ═══════════════════════════════════════════════════════════════
// Draw Dialog Box (bottom center, typewriter text)
// ═══════════════════════════════════════════════════════════════
void CampaignManager::drawDialog(int W, int H) {
    if (!dialogActive || displayedDialogText.empty()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Dialog box dimensions
    float boxW = 600, boxH = 65;
    float bx = (W - boxW) / 2.0f;
    float by = H - 160;

    // Dark translucent background
    glColor4f(0.02f, 0.02f, 0.06f, 0.85f);
    cmFillRect(bx, by, boxW, boxH);

    // Border with radio-green tint
    glColor4f(0.2f, 0.8f, 0.3f, 0.7f);
    glLineWidth(2.0f);
    cmOutlineRect(bx, by, boxW, boxH);
    glLineWidth(1.0f);

    // Radio icon (small antenna symbol)
    glColor4f(0.3f, 0.9f, 0.4f, 0.9f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex2f(bx + 18, by + boxH - 12);
    glVertex2f(bx + 18, by + 12);
    glEnd();
    // Antenna arcs
    glBegin(GL_LINE_STRIP);
    for(int i = 0; i < 8; i++){
        float a = -1.0f + i * 0.25f;
        glVertex2f(bx + 18 + 8*sinf(a), by + 10 + 4*cosf(a));
    }
    glEnd();
    glLineWidth(1.0f);

    // Typewriter text — split into two lines if needed
    glColor4f(0.3f, 1.0f, 0.4f, 1.0f);

    std::string line1, line2;
    if ((int)displayedDialogText.size() > 60) {
        // Find a space near char 60 to break
        size_t breakPos = displayedDialogText.rfind(' ', 60);
        if (breakPos == std::string::npos) breakPos = 60;
        line1 = displayedDialogText.substr(0, breakPos);
        line2 = displayedDialogText.substr(breakPos + 1);
    } else {
        line1 = displayedDialogText;
    }

    cmText(bx + 35, by + 22, GLUT_BITMAP_HELVETICA_12, line1.c_str());
    if (!line2.empty()) {
        cmText(bx + 35, by + 40, GLUT_BITMAP_HELVETICA_12, line2.c_str());
    }

    // Blinking cursor at end of text
    if ((dialogTimer / 15) % 2 == 0) {
        float cursorX = bx + 35;
        const char* lastLine = line2.empty() ? line1.c_str() : line2.c_str();
        for(const char* c = lastLine; *c; c++)
            cursorX += glutBitmapWidth(GLUT_BITMAP_HELVETICA_12, *c);
        float cursorY = line2.empty() ? by + 14 : by + 32;
        glColor4f(0.3f, 1.0f, 0.4f, 0.8f);
        cmFillRect(cursorX + 2, cursorY, 8, 12);
    }

    // Fade-out effect near end
    if (dialogTimer > dialogDuration - 60) {
        float fadeAlpha = (float)(dialogDuration - dialogTimer) / 60.0f;
        // Reduce overall alpha... handled by timing above
    }

    glDisable(GL_BLEND);
}

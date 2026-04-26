#pragma once
#include <string>
#include <vector>
#include <cmath>
#include <GL/freeglut.h>

// ─── Campaign Acts ───
enum CampaignAct {
    ACT_SCAVENGE  = 0,   // Collect fuel cans at waypoints
    ACT_HOLDOUT   = 1,   // Survive the horde at the generator
    ACT_ESCAPE    = 2,   // Reach the extraction zone
    ACT_VICTORY   = 3    // Player won the campaign
};

// ─── Waypoint (objective marker) ───
struct Waypoint {
    float x, y;
    float radius;
    bool  collected;
    const char* label;

    Waypoint() : x(0),y(0),radius(40),collected(false),label("") {}
    Waypoint(float x, float y, const char* lbl)
        : x(x),y(y),radius(40),collected(false),label(lbl) {}
};

// ─── Radio Dialog Message ───
struct RadioMessage {
    std::string text;
    int   triggerFrame;    // frame at which to start showing
    bool  played;

    RadioMessage() : triggerFrame(0), played(false) {}
    RadioMessage(const std::string& t, int frame)
        : text(t), triggerFrame(frame), played(false) {}
};

// ═══════════════════════════════════════════════════════════════
// CampaignManager
// ═══════════════════════════════════════════════════════════════
class CampaignManager {
private:
    CampaignAct currentAct;
    int actTimer;             // Frames elapsed in current act
    int totalFrames;          // Total frames since campaign start

    // ── Act 1: Scavenge ──
    std::vector<Waypoint> fuelCans;
    int fuelsCollected;
    int fuelsRequired;

    // ── Act 2: Holdout ──
    int holdoutDuration;      // frames (2 min = 12000 frames at 100fps... let's use ~3600 at 30fps)
    int holdoutTimer;
    bool generatorActive;
    int hordeSpawnTimer;
    int hordeSpawnInterval;

    // ── Act 3: Escape ──
    Waypoint extractionZone;
    bool playerReachedExtraction;

    // ── Radio/Dialog System ──
    std::string currentDialogText;   // Full text to show
    std::string displayedDialogText; // Typewriter-revealed portion
    int   dialogCharIndex;           // How many chars revealed
    int   dialogTimer;               // Frames since dialog started
    int   dialogDuration;            // Total frames to show
    bool  dialogActive;
    int   typewriterSpeed;           // Frames per character reveal

    // ── Visual ──
    float pulseTimer;

public:
    CampaignManager();

    // Core update — called every frame from Game::timer
    // Returns: number of enemies to spawn this frame (for holdout horde)
    int update(float playerX, float playerY, float playerW, float playerH,
               int screenW, int screenH);

    // Drawing
    void drawObjectives(int screenW, int screenH);
    void drawDialog(int screenW, int screenH);
    void drawWaypoints();

    // Dialog system
    void showRadioMessage(const std::string& text, int durationFrames = 400);

    // Getters
    CampaignAct getAct() const { return currentAct; }
    bool isHoldoutActive() const { return currentAct == ACT_HOLDOUT && generatorActive; }
    bool isCampaignWon() const { return currentAct == ACT_VICTORY; }
    int  getHoldoutTimeLeft() const { return holdoutDuration - holdoutTimer; }
    float getHoldoutProgress() const { return (float)holdoutTimer / (float)holdoutDuration; }

    // Initialize waypoints based on screen size
    void initWaypoints(int screenW, int screenH);
    bool waypointsInitialized() const { return !fuelCans.empty(); }
};

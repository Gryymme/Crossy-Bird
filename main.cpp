/*
 * CROSSY BIRD (Final Version: Scrolling BG + Screen Shake)
 * Features: Infinite Background, Texture Mapping, Sprite UI, Screen Shake FX
 */

#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>

// Image Loader Implementation
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

// Global Variables & Game State
enum GameState { MENU, COUNTDOWN, GAME, GAMEOVER };
GameState currentState = MENU;

// Window Dimensions
int windowWidth = 800;
int windowHeight = 600;

// Game Variables
int score = 0;
int highScore = 0;
float gameSpeed = 0.005f; 
int countdownTimer = 3; 

// Input State
bool keyState[256] = {false};

// Physics Variables
float birdX = 0.0f;
float birdY = 0.0f;
float birdVelY = 0.0f;

const float GRAVITY = -0.0008f; 
const float JUMP_FORCE = 0.018f; 
const float MOVE_SPEED = 0.015f; 
const float BIRD_SIZE = 0.08f;

// Screen Shake Variables
float currentShake = 0.0f; 
const float SHAKE_DECAY = 0.90f; 

// Background Scrolling Variables
float bgX1 = 0.0f; 
float bgX2 = 2.0f; 
float bgScrollSpeed = 0.0015f; 

// --- Texture Variables ---
GLuint bgTextureID1; 
GLuint bgTextureID2; 
GLuint titleTextureID; 
GLuint gameOverTextureID;
GLuint controlsTextureID; 

// Countdown Textures
GLuint texCount3;
GLuint texCount2;
GLuint texCount1;

// Score Textures (0-9)
GLuint texScoreFirst[10];
GLuint texScoreSecond[10];

// High Score Textures (0-9)
GLuint texHSFirst[10];
GLuint texHSSecond[10];

// Obstacle Types
enum ObstacleType { BRANCH, CLOUD };

struct Obstacle {
    ObstacleType type;
    float x;
    float y;
    float gapY; 
    bool passed;
    float speed; 
};
vector<Obstacle> obstacles;

// Forward Declarations
void timerCountdown(int value);
void timerGameLoop(int value);
void timerMenuRefresh(int value);
void timerShake(int value);

// --- Utility Functions ---
void drawString(float x, float y, void *font, const char *format, int value) {
    char buffer[50];
    sprintf(buffer, format, value);
    glRasterPos2f(x, y);
    for (const char *c = buffer; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}

void renderBitmapString(float x, float y, void *font, const char *string) {
    const char *c;
    glRasterPos2f(x, y);
    for (c = string; *c != '\0'; c++) {
        glutBitmapCharacter(font, *c);
    }
}

void drawCircle(float r, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.0f, 0.0f); 
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * 3.14159f * float(i) / float(segments);
        float dx = r * cosf(theta);
        float dy = r * sinf(theta);
        glVertex2f(dx, dy);
    }
    glEnd();
}

// --- Texture Loading Helper ---
GLuint loadSingleTexture(const char* filename) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
    
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    } else {
        cout << "Failed to load texture: " << filename << endl;
    }
    stbi_image_free(data);
    return textureID;
}

void loadAllTextures() {
    // Backgrounds
    bgTextureID1 = loadSingleTexture("assets/background.png");
    bgTextureID2 = loadSingleTexture("assets/background2.png");

    // UI
    titleTextureID = loadSingleTexture("assets/title.png");
    gameOverTextureID = loadSingleTexture("assets/gameover.png");
    controlsTextureID = loadSingleTexture("assets/pregamecontrols.png");

    // Countdown
    texCount3 = loadSingleTexture("assets/pregamecd_3.png");
    texCount2 = loadSingleTexture("assets/pregamecd_2.png");
    texCount1 = loadSingleTexture("assets/pregamecd_1.png");

    char filename[50];

    // Score Digits
    for (int i = 0; i < 10; i++) {
        sprintf(filename, "assets/firstdigit_%d.png", i);
        texScoreFirst[i] = loadSingleTexture(filename);
        
        sprintf(filename, "assets/seconddigit_%d.png", i);
        texScoreSecond[i] = loadSingleTexture(filename);
    }

    // High Score Digits
    for (int i = 0; i < 10; i++) {
        sprintf(filename, "assets/hsfirstdigit_%d.png", i);
        texHSFirst[i] = loadSingleTexture(filename);
        
        sprintf(filename, "assets/hsseconddigit_%d.png", i);
        texHSSecond[i] = loadSingleTexture(filename);
    }
}

// --- Drawing Functions ---

void drawFullScreenTexture(GLuint textureID) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glColor3f(1.0f, 1.0f, 1.0f); 
    
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f); 
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);  
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);   
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);  
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
}

void updateBackground() {
    if (currentState == GAMEOVER) return; 

    bgX1 -= bgScrollSpeed;
    bgX2 -= bgScrollSpeed;

    if (bgX1 <= -2.0f) bgX1 = bgX2 + 2.0f;
    if (bgX2 <= -2.0f) bgX2 = bgX1 + 2.0f;
}

void drawScrollingBackground() {
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f); 

    glBindTexture(GL_TEXTURE_2D, bgTextureID1);
    glPushMatrix();
    glTranslatef(bgX1, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f); 
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);  
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);   
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);  
    glEnd();
    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, bgTextureID2);
    glPushMatrix();
    glTranslatef(bgX2, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f); 
        glTexCoord2f(1.0f, 0.0f); glVertex2f(1.0f, -1.0f);  
        glTexCoord2f(1.0f, 1.0f); glVertex2f(1.0f, 1.0f);   
        glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, 1.0f);  
    glEnd();
    glPopMatrix();

    glDisable(GL_TEXTURE_2D); 
}

void drawCloudShape() {
    glColor3f(1.0f, 1.0f, 1.0f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f);
    drawCircle(0.08f, 10);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.1f, 0.02f, 0.0f);
    drawCircle(0.09f, 10);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.08f, -0.01f, 0.0f);
    drawCircle(0.07f, 10);
    glPopMatrix();
}

void drawBird() {
    glPushMatrix();
    glTranslatef(birdX, birdY, 0.0f);
    
    // Bird Body
    glColor3f(1.0f, 1.0f, 0.0f); 
    glBegin(GL_QUADS);
        glVertex2f(-BIRD_SIZE, -BIRD_SIZE);
        glVertex2f(BIRD_SIZE, -BIRD_SIZE);
        glVertex2f(BIRD_SIZE, BIRD_SIZE);
        glVertex2f(-BIRD_SIZE, BIRD_SIZE);
    glEnd();

    // Bird Beak
    glColor3f(1.0f, 0.5f, 0.0f);
    glBegin(GL_TRIANGLES);
        glVertex2f(BIRD_SIZE, 0.02f);
        glVertex2f(BIRD_SIZE + 0.04f, -0.02f);
        glVertex2f(BIRD_SIZE, -0.04f);
    glEnd();
    
    // Eye
    glColor3f(0.0f, 0.0f, 0.0f);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
        glVertex2f(BIRD_SIZE - 0.02f, 0.02f);
    glEnd();

    glPopMatrix();
}

void drawObstacles() {
    for (size_t i = 0; i < obstacles.size(); ++i) {
        Obstacle& obs = obstacles[i];

        if (obs.type == BRANCH) {
            glBegin(GL_QUADS);
                // Top
                glColor3f(0.4f, 0.2f, 0.0f); 
                glVertex2f(obs.x - 0.1f, obs.gapY + 0.4f); 
                glColor3f(0.6f, 0.4f, 0.2f); 
                glVertex2f(obs.x + 0.1f, obs.gapY + 0.4f);
                glColor3f(0.6f, 0.4f, 0.2f); 
                glVertex2f(obs.x + 0.1f, 1.0f);
                glColor3f(0.4f, 0.2f, 0.0f); 
                glVertex2f(obs.x - 0.1f, 1.0f);
                
                // Bottom
                glColor3f(0.4f, 0.2f, 0.0f);
                glVertex2f(obs.x - 0.1f, -1.0f);
                glColor3f(0.6f, 0.4f, 0.2f);
                glVertex2f(obs.x + 0.1f, -1.0f);
                glColor3f(0.6f, 0.4f, 0.2f);
                glVertex2f(obs.x + 0.1f, obs.gapY - 0.4f);
                glColor3f(0.4f, 0.2f, 0.0f);
                glVertex2f(obs.x - 0.1f, obs.gapY - 0.4f);
            glEnd();
        }
        else if (obs.type == CLOUD) {
            glPushMatrix();
            glTranslatef(obs.x, obs.y, 0.0f);
            glScalef(1.5f, 1.5f, 1.0f);
            drawCloudShape();
            glPopMatrix();
        }
    }
}

void drawUI() {
    glColor3f(1.0f, 1.0f, 1.0f); 
    void* font = GLUT_BITMAP_HELVETICA_18; 
    
    if (currentState == MENU) {
        drawFullScreenTexture(titleTextureID);
    } 
    else if (currentState == COUNTDOWN) {
        drawFullScreenTexture(controlsTextureID);

        if (countdownTimer == 3) {
            drawFullScreenTexture(texCount3);
        } else if (countdownTimer == 2) {
            drawFullScreenTexture(texCount2);
        } else if (countdownTimer == 1) {
            drawFullScreenTexture(texCount1);
        }
    }
    else if (currentState == GAME) {
        glColor3f(0.0f, 0.0f, 0.0f); 
        drawString(-0.95f, 0.9f, font, "Score: %d", score);
    }
    else if (currentState == GAMEOVER) {
        drawFullScreenTexture(gameOverTextureID);

        int displayScore = (score > 99) ? 99 : score;
        int displayHS = (highScore > 99) ? 99 : highScore;

        drawFullScreenTexture(texScoreFirst[displayScore / 10]);
        drawFullScreenTexture(texScoreSecond[displayScore % 10]);
        drawFullScreenTexture(texHSFirst[displayHS / 10]);
        drawFullScreenTexture(texHSSecond[displayHS % 10]);
    }
}

// --- Logic Functions ---
void loadHighScore() {
    ifstream file("highscore.txt");
    if (file.is_open()) {
        file >> highScore;
        file.close();
    }
}

void saveHighScore() {
    ofstream file("highscore.txt");
    if (file.is_open()) {
        file << highScore;
        file.close();
    }
}

void init() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); 
    loadHighScore();
    loadAllTextures(); 
    
    bgX1 = 0.0f;
    bgX2 = 2.0f;

    srand(time(NULL));
}

void resetGame() {
    birdX = 0.0f;
    birdY = 0.0f;
    birdVelY = 0.0f;
    score = 0;
    gameSpeed = 0.005f; 
    
    bgScrollSpeed = 0.0015f;

    obstacles.clear();
    Obstacle obs1;
    obs1.type = BRANCH;
    obs1.x = 1.5f; obs1.y = 0.0f; obs1.gapY = 0.0f; obs1.passed = false; obs1.speed = gameSpeed;
    obstacles.push_back(obs1);
}

bool checkCollision() {
    if (birdY + BIRD_SIZE > 1.0f || birdY - BIRD_SIZE < -1.0f || 
        birdX + BIRD_SIZE > 1.0f || birdX - BIRD_SIZE < -1.0f) {
        return true;
    }

    for (size_t i = 0; i < obstacles.size(); ++i) {
        Obstacle& obs = obstacles[i];
        
        if (obs.type == BRANCH) {
            float obsW = 0.1f; 
            
            bool xOverlap = (birdX + BIRD_SIZE > obs.x - obsW) && (birdX - BIRD_SIZE < obs.x + obsW);
            
            if (xOverlap) {
                bool hitTop = (birdY + BIRD_SIZE > obs.gapY + 0.4f);
                bool hitBottom = (birdY - BIRD_SIZE < obs.gapY - 0.4f);
                if (hitTop || hitBottom) return true;
            }
        }
        else if (obs.type == CLOUD) {
            float cloudSize = 0.15f; 
            bool xOverlap = (birdX + BIRD_SIZE > obs.x - cloudSize) && (birdX - BIRD_SIZE < obs.x + cloudSize);
            bool yOverlap = (birdY + BIRD_SIZE > obs.y - cloudSize) && (birdY - BIRD_SIZE < obs.y + cloudSize);
            
            if (xOverlap && yOverlap) return true;
        }
    }
    return false;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Apply Shake to the Game World
    glPushMatrix();
    if (currentShake > 0.0f) {
        float shakeX = ((rand() % 100) / 50.0f - 1.0f) * currentShake;
        float shakeY = ((rand() % 100) / 50.0f - 1.0f) * currentShake;
        glTranslatef(shakeX, shakeY, 0.0f);
    }

    // 1. Backgrounds
    drawScrollingBackground();

    // 2. Game Objects
    if (currentState == MENU) {
        // Background only
    }
    else if (currentState == COUNTDOWN) {
        // Background only (Controls overlay handled in UI)
    }
    else if (currentState == GAME || currentState == GAMEOVER) {
        drawBird();
        drawObstacles();
    }

    glPopMatrix(); // Stop shaking for UI

    // 3. UI (Static)
    drawUI();
    
    glutSwapBuffers();
}

void updateInput() {
    if (currentState != GAME) return;

    if (keyState['a'] || keyState['A']) {
        birdX -= MOVE_SPEED;
    }
    if (keyState['d'] || keyState['D']) {
        birdX += MOVE_SPEED;
    }
    if (keyState['s'] || keyState['S']) {
        birdY -= MOVE_SPEED;
    }
}

void updateGameLogic() {
    updateInput();

    birdVelY += GRAVITY;
    birdY += birdVelY;

    for (size_t i = 0; i < obstacles.size(); ++i) {
        if (obstacles[i].type == BRANCH) {
            obstacles[i].x -= obstacles[i].speed; 
        } else {
            obstacles[i].y -= obstacles[i].speed; 
        }
    }

    if (!obstacles.empty()) {
        float lastBranchX = -99.0f;
        for (int i = obstacles.size() - 1; i >= 0; --i) {
            if (obstacles[i].type == BRANCH) {
                lastBranchX = obstacles[i].x;
                break;
            }
        }

        if (lastBranchX < 0.8f) { 
             float newGap = (rand() % 100 / 100.0f) - 0.5f; 
             Obstacle newBranch;
             newBranch.type = BRANCH;
             newBranch.x = 1.8f; 
             newBranch.y = 0.0f;
             newBranch.gapY = newGap;
             newBranch.passed = false;
             newBranch.speed = gameSpeed;
             obstacles.push_back(newBranch);
             
             if (rand() % 100 < 75) { 
                 Obstacle newCloud;
                 newCloud.type = CLOUD;
                 newCloud.x = ((rand() % 200) / 100.0f) - 1.0f; 
                 newCloud.y = 1.2f; 
                 newCloud.gapY = 0.0f;
                 newCloud.passed = false;
                 newCloud.speed = 0.008f; 
                 obstacles.push_back(newCloud);
             }
             
             gameSpeed += 0.0002f; 
        }
    }
    
    if (!obstacles.empty()) {
        Obstacle& first = obstacles[0];
        if ((first.type == BRANCH && first.x < -1.5f) || 
            (first.type == CLOUD && first.y < -1.5f)) {
            obstacles.erase(obstacles.begin());
        }
    }

    for (size_t i = 0; i < obstacles.size(); ++i) {
        if (!obstacles[i].passed) {
            if (obstacles[i].type == BRANCH && birdX > obstacles[i].x) {
                score++;
                obstacles[i].passed = true;
            }
        }
    }

    if (checkCollision()) {
        currentState = GAMEOVER;
        currentShake = 0.05f; // Trigger Shake
        glutTimerFunc(16, timerShake, 0); // Start Shake Decay
        if (score > highScore) {
            highScore = score;
            saveHighScore();
        }
    }
}

void timerShake(int value) {
    if (currentShake > 0.001f) {
        currentShake *= SHAKE_DECAY;
        glutTimerFunc(16, timerShake, 0);
    } else {
        currentShake = 0.0f;
    }
    glutPostRedisplay();
}

void timerCountdown(int value) {
    // Background moves in countdown
    updateBackground();

    if (currentState == COUNTDOWN) {
        countdownTimer--;
        if (countdownTimer <= 0) {
            currentState = GAME;
            resetGame();
            glutTimerFunc(16, timerGameLoop, 0);
        } else {
            glutTimerFunc(1000, timerCountdown, 0);
        }
    }
    glutPostRedisplay();
}

void timerGameLoop(int value) {
    if (currentState == GAME) {
        updateBackground(); 
        updateGameLogic();
        glutTimerFunc(16, timerGameLoop, 0);
        glutPostRedisplay();
    }
}

void timerMenuRefresh(int value) {
    if (currentState == MENU) {
        updateBackground();
        glutTimerFunc(16, timerMenuRefresh, 0);
    }
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    keyState[key] = true; 
    
    if (key == 32) { 
        if (currentState == MENU) {
            currentState = COUNTDOWN;
            countdownTimer = 3; 
            glutTimerFunc(1000, timerCountdown, 0); 
            glutPostRedisplay();
        }
        else if (currentState == GAMEOVER) {
            currentState = MENU;
            glutTimerFunc(16, timerMenuRefresh, 0);
            glutPostRedisplay();
        }
    }

    if (currentState == GAME && (key == 'w' || key == 'W')) {
        birdVelY = JUMP_FORCE;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    keyState[key] = false; 
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Crossy Bird - C++ OpenGL");

    GLenum err = glewInit();
    if (err != GLEW_OK) {
        cerr << "GLEW Init Failed" << endl;
        return -1;
    }

    init();

    glutDisplayFunc(display);
    
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    
    glutTimerFunc(16, timerMenuRefresh, 0);

    glutMainLoop();
    return 0;
}

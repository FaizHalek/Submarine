#include <stdio.h>
#include "raylib.h"
#include <stdlib.h>  
#include <math.h>    
#include <float.h>  
#include <errno.h>
#include <string.h>

// gcc main.c -o main.exe -I -L -lraylib -lopengl32 -lgdi32 -lwinmm
// ./main.exe

typedef struct GameConfig {
    int screenWidth;
    int screenHeight;
    float heavyBulletCooldown;
    float shooterCooldown;
    float bossShootCooldown;
    float bossSpawnCooldown;
    int frameWidth;
    int frameHeight;
    int maxEnemies;
    int maxWaves;
    int maxEnemyBullets;
    int maxBossBullets;
    float frameSpeed;
    int frameCount;
    float waveTime;
    float waveSpeed;
    int waveHeight;
    int numWavePoints;
    int numWaves;
    Font customFont;
} GameConfig;

typedef struct Submarine {
    Rectangle rect;          
    Rectangle hitbox;        
    int speed;
    int health;
    int maxHealth;
    float frameTime;
    int currentFrame;
    bool facingLeft;
    float energy;
} Submarine;

typedef struct Bullet {
    Rectangle rect;
    bool active;
    int damage;
} Bullet;

typedef struct Enemy {
    Rectangle rect;
    int speed;
    int health;
    int maxHealth;
    bool active;
    bool isShooter;
    float shootTimer;
    int moveDirection;
    bool isBoss;
    float spawnTimer;
    float distanceMoved;
} Enemy;

typedef struct EnemyBullet {
    Rectangle rect;
    bool active;
} EnemyBullet;

// State of the game
typedef enum GameState {
    STATE_MENU,
    STATE_LEVEL_SELECTION,
    STATE_PLAYING,
    STATE_OPTIONS,
    STATE_BUFF_SELECTION,
    STATE_BUFF_SELECTION_2,
    STATE_VICTORY  
} GameState;

// Function prototypes
void ResetEnemies(Enemy enemies[], int enemyCount, int wave, const GameConfig* config);
bool CheckBossWaveComplete(Enemy enemies[], const GameConfig* config);
bool CheckCollisionWithWalls(float newX, float subY, Rectangle subRect, const GameConfig* config);
// Function prototypes are essential in C programming as they inform the compiler about the function's name, return type, and parameters before the function is actually defined. This allows for better organization of code and helps avoid issues related to function declarations and definitions. In your code, these prototypes indicate that the functions will be implemented later in the file, and they will be used to manage enemy behavior, check for wave completion, and handle collision detection.

// Function to get the bullet position based on the submarine's position
Vector2 GetBulletPosition(Submarine sub, float bulletWidth) {
    return (Vector2){ 
        sub.rect.x + (sub.rect.width / 2) - (bulletWidth / 2),  
        sub.rect.y  
    };
}

void ReadLowestTimes(float lowestTimes[], int maxLevels) {
    FILE *file = fopen("lowestTime.txt", "r");  
    if (file) {
        for (int i = 0; i < maxLevels; i++) {
            fscanf(file, "%*[^:]: %f\n", &lowestTimes[i]);  
        }
        fclose(file);  
    } else {
        for (int i = 0; i < maxLevels; i++) {
            lowestTimes[i] = FLT_MAX;  
        }
    }
}

void WriteLowestTimes(float lowestTimes[], int maxLevels) {
    FILE *file = fopen("lowestTime.txt", "w");  
    if (file == NULL) {
        printf("Error opening file: %s\n", strerror(errno));
        return;
    }
    
    if (fprintf(file, "Easy: %.2f\n", lowestTimes[0]) < 0 ||
        fprintf(file, "Medium: %.2f\n", lowestTimes[1]) < 0 ||
        fprintf(file, "Hard: %.2f\n", lowestTimes[2]) < 0) {
        printf("Error writing to file: %s\n", strerror(errno));
        fclose(file);
        return;
    }
    
    if (fclose(file) != 0) {
        printf("Error closing file: %s\n", strerror(errno));
        return;
    }
}

int main(void)
{
    // Initialize configuration
    GameConfig config = {
        .screenWidth = 600,
        .screenHeight = 800,
        .heavyBulletCooldown = 3.0f,
        .shooterCooldown = 1.5f,
        .bossShootCooldown = 0.5f,
        .bossSpawnCooldown = 5.0f,
        .frameWidth = 64,
        .frameHeight = 64,
        .maxEnemies = 50,
        .maxWaves = 5,
        .maxEnemyBullets = 50,
        .maxBossBullets = 100,
        .frameSpeed = 0.1f,
        .frameCount = 1,
        .waveTime = 0.0f,
        .waveSpeed = 2.0f,
        .waveHeight = 20.0f,
        .numWavePoints = 30,
        .numWaves = 5,
    };

    // Initialize window with config values
    InitWindow(config.screenWidth, config.screenHeight, "Submarine Strike");

    // Initialize audio device
    InitAudioDevice();  

    // Load music
    Music backgroundMusic = LoadMusicStream("music.ogg");
    PlayMusicStream(backgroundMusic);  

    // Initialize volume variable
    float musicVolume = 0.1f;  // Default volume set to 50%
    SetMusicVolume(backgroundMusic, musicVolume);  

    config.customFont = LoadFont("fonts/Harmonic.ttf");

    // Create enemy bullets array and initialize it
    EnemyBullet enemyBullets[config.maxEnemyBullets];
    for(int i = 0; i < config.maxEnemyBullets; i++) {
        enemyBullets[i].active = false;
        enemyBullets[i].rect = (Rectangle){0, 0, 0, 0};
    }

    float heavyBulletTimer = 0.0f;

    // Load textures
    Texture2D menuButtonTexture = LoadTexture("images/Button_Blue_3Slides.png");
    Texture2D menuButtonPressedTexture = LoadTexture("images/Button_Blue_3Slides_Pressed.png");
    Texture2D submarineSheet = LoadTexture("images/submarine.png");
    Texture2D rocketTexture1 = LoadTexture("images/left_click_1.png");
    Texture2D rocketTexture2 = LoadTexture("images/left_click_2.png");
    Texture2D rocketTexture3 = LoadTexture("images/left_click_3.png");
    Texture2D rocketTexture4 = LoadTexture("images/left_click_4.png");
    Texture2D rocketTexture5 = LoadTexture("images/left_click_5.png");
    Texture2D startMenuTexture = LoadTexture("images/start_menu.png");
    Texture2D backgroundMenuTexture = LoadTexture("images/background_menu.png");
    Texture2D backgroundMenuTexture2 = LoadTexture("images/game_background.png");
    Texture2D normalEnemyFront = LoadTexture("images/normalEnemy_front.png");
    Texture2D normalEnemyBack = LoadTexture("images/normalEnemy_back.png");
    Texture2D normalEnemy2Front = LoadTexture("images/normalEnemy2_front.png");
    Texture2D normalEnemy3Front = LoadTexture("images/normalEnemy3_front.png");
    Texture2D normalEnemy2Back = LoadTexture("images/normalEnemy2_back.png");
    Texture2D normalEnemy3Back = LoadTexture("images/normalEnemy3_back.png");
    Texture2D bossTexture = LoadTexture("images/boss_pic.png");
    Texture2D leftClickAnimationTexture = LoadTexture("images/left_click_animation.png");

    Submarine sub = {
        .rect = {
            config.screenWidth / 2 - config.frameWidth / 2, 
            config.screenHeight - 120,
            (float)config.frameWidth * 1.0f,
            (float)config.frameHeight * 2.0f
        },
        .hitbox = {
            config.screenWidth / 2 - config.frameWidth / 4,  
            config.screenHeight - 100,                 
            (float)config.frameWidth * 1.5f,              
            (float)config.frameHeight * 1.5f
        },
        .speed = 200,
        .health = 100,
        .maxHealth = 100,
        .frameTime = 0.0f,
        .currentFrame = 0,
        .facingLeft = false,
        .energy = 100.0f
    };

    // Create bullets
    Bullet bullets[10];
    for (int i = 0; i < 10; i++) {
        bullets[i].active = false;
    }

    // Create enemies array using config
    Enemy enemies[config.maxEnemies];
    int wave = 1;
    int maxEnemies = wave * 5;
    
    // Update function calls to pass config
    ResetEnemies(enemies, maxEnemies, wave, &config);

    // Game variables
    float timer = 0.0f;  
    bool gameOver = false;
    bool victory = false;
    bool startScreen = true;

    int bulletIndex = 0;  
    int score = 0;  

    float backgroundScrollX = 0.0f;
    float scrollSpeed = 2.0f;
    int scrollDirection = 1;  

    GameState currentState = STATE_MENU;  

    // Difficulty level variable
    int difficultyLevel = 1;  

    SetTargetFPS(60); 
    
    // Define animation variables
    const int totalFrames = 5;  
    int currentFrame = 0;      
    float frameTime = 0.0f;     
    float updateTime = 0.1f;    

    // Define animation variables for right-click
    const int totalRightClickFrames = 5;  
    Texture2D rocketTextures[5];          
    int currentRightClickFrame = 0;       
    float rightClickFrameTime = 0.0f;     
    float rightClickUpdateTime = 0.1f;     

    // Load the rocket textures at the beginning of your main function
    rocketTextures[0] = LoadTexture("images/left_click_1.png");
    rocketTextures[1] = LoadTexture("images/left_click_2.png");
    rocketTextures[2] = LoadTexture("images/left_click_3.png");
    rocketTextures[3] = LoadTexture("images/left_click_4.png");
    rocketTextures[4] = LoadTexture("images/left_click_5.png");

    float lowestTimes[3];  
    ReadLowestTimes(lowestTimes, 3);  

    printf("Easy: %.2f, Medium: %.2f, Hard: %.2f\n", lowestTimes[0], lowestTimes[1], lowestTimes[2]);

    // Load the button textures at the beginning of your main function
    Texture2D backButtonTexture = LoadTexture("images/Button_Blue.png");
    Texture2D backButtonPressedTexture = LoadTexture("images/Button_Blue_Pressed.png");
    Texture2D restartButtonTexture = LoadTexture("images/Button_Blue_3Slides.png");
    Texture2D restartButtonPressedTexture = LoadTexture("images/Button_Blue_3Slides_Pressed.png");
    Texture2D exitButtonTexture = LoadTexture("images/Button_Blue_3Slides.png");
    Texture2D exitButtonPressedTexture = LoadTexture("images/Button_Blue_3Slides_Pressed.png");

    // Add scrolling variables for options background
    float optionsScrollX = 0.0f;  
    float optionsScrollSpeed = 2.0f;  

    

    // water texture
    Texture2D waterTexture = LoadTexture("images/water_texture.png"); 
    float waterScrollSpeed = 0.5f; 
    float waterOffset = 0.0f; 

    // Add buff tracking variables
    bool hasLifestealBuff = false;
    bool hasUnlimitedRightClickBuff = false;
    bool hasUnlimitedEnergyBuff = false;

    // Main game loop
    while (!WindowShouldClose()) {
        // Update music stream
        UpdateMusicStream(backgroundMusic);

        if (currentState == STATE_MENU) {
            BeginDrawing();
            ClearBackground(BLACK);

            // Update scroll position
            backgroundScrollX += scrollDirection * scrollSpeed;

            // Calculate the maximum scroll position (image width minus screen width)
            float maxScroll = backgroundMenuTexture.width - config.screenWidth;

            // Check bounds and reverse direction
            if (backgroundScrollX >= maxScroll) {
                backgroundScrollX = maxScroll;
                scrollDirection = -1;
            } else if (backgroundScrollX <= 0) {
                backgroundScrollX = 0;
                scrollDirection = 1;
            }

            // Draw background
            DrawTexturePro(backgroundMenuTexture,
                (Rectangle){ 
                    backgroundScrollX,                    
                    0,                                    
                    config.screenWidth,                   
                    backgroundMenuTexture.height           
                },
                (Rectangle){ 
                    0,                                    
                    0,                                    
                    config.screenWidth,                   
                    config.screenHeight                   
                },
                (Vector2){ 0, 0 },
                0.0f,
                WHITE);

            // Define button rectangles
            Rectangle startButton = { config.screenWidth / 2 - 100, config.screenHeight / 2 + 0, 200, 50 };
            Rectangle optionsButton = { config.screenWidth / 2 - 100, config.screenHeight / 2 + 70, 200, 50 };
            Rectangle exitButton = { config.screenWidth / 2 - 100, config.screenHeight / 2 + 140, 200, 50 };

            // Check for mouse position
            Vector2 mousePos = GetMousePosition();

            // Draw buttons with hover effect
            if (CheckCollisionPointRec(mousePos, startButton)) {
                DrawTexture(menuButtonPressedTexture, startButton.x, startButton.y, WHITE);
            } else {
                DrawTexture(menuButtonTexture, startButton.x, startButton.y, WHITE);
            }

            if (CheckCollisionPointRec(mousePos, optionsButton)) {
                DrawTexture(menuButtonPressedTexture, optionsButton.x, optionsButton.y, WHITE);
            } else {
                DrawTexture(menuButtonTexture, optionsButton.x, optionsButton.y, WHITE);
            }

            if (CheckCollisionPointRec(mousePos, exitButton)) {
                DrawTexture(menuButtonPressedTexture, exitButton.x, exitButton.y, WHITE);
            } else {
                DrawTexture(menuButtonTexture, exitButton.x, exitButton.y, WHITE);
            }

            // Draw button text
            DrawText("Start", startButton.x + 60, startButton.y + 10, 20, WHITE);
            DrawText("Settings", optionsButton.x + 60, optionsButton.y + 10, 20, WHITE);
            DrawText("Exit", exitButton.x + 80, exitButton.y + 10, 20, WHITE);

            // Check for button clicks
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mousePos, startButton)) {
                    currentState = STATE_LEVEL_SELECTION; 
                } else if (CheckCollisionPointRec(mousePos, optionsButton)) {
                    currentState = STATE_OPTIONS; // Go to options
                } else if (CheckCollisionPointRec(mousePos, exitButton)) {
                    CloseWindow(); // Exit the game
                }
            }

            DrawTextEx(config.customFont, "SUBMARINE STRIKE", (Vector2){config.screenWidth / 2 - 260, config.screenHeight / 2 - 100}, 70, 2, DARKBLUE);

            EndDrawing();
        } else if (currentState == STATE_LEVEL_SELECTION) {
            backgroundScrollX += scrollSpeed;
            if (backgroundScrollX >= config.screenWidth) {
                backgroundScrollX = 0;  
            }

            BeginDrawing();
            
            // Draw the scrolling background
            DrawTexturePro(backgroundMenuTexture,
                (Rectangle){ backgroundScrollX, 0, config.screenWidth, backgroundMenuTexture.height },
                (Rectangle){ 0, 0, config.screenWidth, config.screenHeight },
                (Vector2){ 0, 0 },
                0.0f,
                WHITE);
            
            // Draw the title
            DrawText("Select Difficulty Level:", config.screenWidth / 2 - 100, config.screenHeight / 2 - 50, 20, GRAY);
            
            // Define clickable areas for each button (easy, medium, hard)
            Rectangle easyButton = { config.screenWidth / 2 - 100, config.screenHeight / 2 + 0, 200, 50 };
            Rectangle mediumButton = { config.screenWidth / 2 - 100, config.screenHeight / 2 + 70, 200, 50 };
            Rectangle hardButton = { config.screenWidth / 2 - 100, config.screenHeight / 2 + 140, 200, 50 };

            // Check for mouse position
            Vector2 mousePos = GetMousePosition();

            // Draw buttons with hover effect
            if (CheckCollisionPointRec(mousePos, easyButton)) {
                DrawTexture(menuButtonPressedTexture, easyButton.x, easyButton.y, WHITE); 
            } else {
                DrawTexture(menuButtonTexture, easyButton.x, easyButton.y, WHITE); 
            }
            DrawText("Easy", easyButton.x + 60, easyButton.y + 10, 20, WHITE); 
            DrawText(TextFormat("Fastest Time: %.2f", lowestTimes[0]), easyButton.x + 200, easyButton.y + 20, 20, GRAY); 

            if (CheckCollisionPointRec(mousePos, mediumButton)) {
                DrawTexture(menuButtonPressedTexture, mediumButton.x, mediumButton.y, WHITE); 
            } else {
                DrawTexture(menuButtonTexture, mediumButton.x, mediumButton.y, WHITE); 
            }
            DrawText("Medium", mediumButton.x + 60, mediumButton.y + 10, 20, WHITE); 
            DrawText(TextFormat("Fastest Time: %.2f", lowestTimes[1]), mediumButton.x + 200, mediumButton.y + 20, 20, GRAY); 

            if (CheckCollisionPointRec(mousePos, hardButton)) {
                DrawTexture(menuButtonPressedTexture, hardButton.x, hardButton.y, WHITE); 
            } else {
                DrawTexture(menuButtonTexture, hardButton.x, hardButton.y, WHITE); 
            }
            DrawText("Hard", hardButton.x + 60, hardButton.y + 10, 20, WHITE); 
            DrawText(TextFormat("Fastest Time: %.2f", lowestTimes[2]), hardButton.x + 200, hardButton.y + 20, 20, GRAY); 

            Rectangle backButton = {10, 10, 60, 55};  

            // Draw the back button with hover effect
            if (CheckCollisionPointRec(mousePos, backButton)) {
                DrawTexture(backButtonPressedTexture, backButton.x, backButton.y, WHITE); 
            } else {
                DrawTexture(backButtonTexture, backButton.x, backButton.y, WHITE); 
            }

            DrawText("←", backButton.x + backButton.width / 2 - 10, backButton.y + backButton.height / 2 - 10, 20, WHITE); 

            // Check for button clicks to exit
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mousePos, backButton)) {
                currentState = STATE_MENU;
            }

            // Check button clicks for difficulty selection
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (CheckCollisionPointRec(mousePos, easyButton)) {
                    difficultyLevel = 1;  
                    currentState = STATE_PLAYING;  
                } else if (CheckCollisionPointRec(mousePos, mediumButton)) {
                    difficultyLevel = 2;  
                    currentState = STATE_PLAYING;  
                } else if (CheckCollisionPointRec(mousePos, hardButton)) {
                    difficultyLevel = 3;  
                    currentState = STATE_PLAYING;  
                }
            }

            EndDrawing();
        } else if (currentState == STATE_PLAYING) {
            // Update the water offset for scrolling
            waterOffset += waterScrollSpeed * GetFrameTime();
            if (waterOffset >= waterTexture.height) {
                waterOffset = 0; // Reset the offset to create a loop
            }

            // Draw the moving water background
            DrawTexturePro(waterTexture,
                (Rectangle){ 0, waterOffset, waterTexture.width, waterTexture.height - waterOffset }, // Top part
                (Rectangle){ 0, 0, config.screenWidth, config.screenHeight }, // Destination rectangle
                (Vector2){ 0, 0 }, // Origin
                0.0f, // Rotation
                WHITE); // Color

            DrawTexturePro(waterTexture,
                (Rectangle){ 0, 0, waterTexture.width, waterOffset }, // Bottom part
                (Rectangle){ 0, config.screenHeight - waterOffset, config.screenWidth, waterOffset }, // Destination rectangle
                (Vector2){ 0, 0 }, // Origin
                0.0f, // Rotation
                WHITE); // Color

            // Update the timer
            timer += GetFrameTime();  

            // Start the game with the selected difficulty level
            // Adjust game parameters based on difficultyLevel
            if (difficultyLevel == 1) {  // Easy
                // Set parameters for easy level
                for (int i = 0; i < maxEnemies; i++) {
                    enemies[i].speed = 1;  
                    enemies[i].maxHealth = 1;  
                }
            } else if (difficultyLevel == 2) {  
                // Set parameters for medium level
                for (int i = 0; i < maxEnemies; i++) {
                    enemies[i].speed = 2;  
                    enemies[i].maxHealth = 2;  
                }
            } else if (difficultyLevel == 3) {  
                // Set parameters for hard level
                for (int i = 0; i < maxEnemies; i++) {
                    enemies[i].speed = 3;  
                    enemies[i].maxHealth = 3;  
                }
            }

            // Regular game drawing
            BeginDrawing();
            
            // Update scroll position for game background
            backgroundScrollX += scrollSpeed;
            if (backgroundScrollX >= backgroundMenuTexture2.width - config.screenWidth) {
                backgroundScrollX = 0;
            }

            // Update wave time
            config.waveTime += GetFrameTime() * config.waveSpeed;

            // Background colour
            DrawRectangle(0, 0, config.screenWidth, config.screenHeight, (Color){0, 105, 148, 255});  

            // Victory screen logic
            if (victory) {
                BeginDrawing();
                DrawTexturePro(backgroundMenuTexture,  // Use the background menu texture
                (Rectangle){ 
                    backgroundScrollX, 
                    0, 
                    config.screenWidth, 
                    backgroundMenuTexture.height 
                },
                (Rectangle){ 
                    0, 
                    0, 
                    config.screenWidth, 
                    config.screenHeight 
                },
                (Vector2){ 0, 0 },
                0.0f,
                WHITE);

                float newTime = timer;
                if (newTime < lowestTimes[difficultyLevel - 1]) {
                    lowestTimes[difficultyLevel - 1] = newTime;
                    WriteLowestTimes(lowestTimes, 3);  // Write the new times to file
                }

                DrawText("VICTORY!", config.screenWidth / 2 - 120, config.screenHeight / 2 - 100, 40, GREEN);
                DrawText("You completed all 5 waves!", config.screenWidth / 2 - 190, config.screenHeight / 2, 30, BLACK);

                DrawText("Press Enter to restart or Esc to exit", config.screenWidth / 2 - 200, config.screenHeight / 2 + 30, 20, BLACK);
                
                EndDrawing();

                if (IsKeyPressed(KEY_ENTER)) {
                    // Write the times to file before resetting
                    WriteLowestTimes(lowestTimes, 3);
                    
                    // Reset all game variables
                    timer = 0.0f;
                    wave = 1;
                    maxEnemies = wave * 5;
                    sub.speed = 200;
                    sub.health = sub.maxHealth;
                    sub.energy = 100.0f;
                    ResetEnemies(enemies, maxEnemies, wave, &config);
                    victory = false;
                    gameOver = false; 
                    currentState = STATE_LEVEL_SELECTION;
                } else if (IsKeyPressed(KEY_ESCAPE)) {
                    CloseWindow(); // Exit the game
                }

                continue;  
            }

            // Game over logic
            if (gameOver) {
                BeginDrawing();
                DrawTexturePro(backgroundMenuTexture, 
                    (Rectangle){ 
                        backgroundScrollX, 
                        0, 
                        config.screenWidth, 
                        backgroundMenuTexture.height 
                    },
                    (Rectangle){ 
                        0, 
                        0, 
                        config.screenWidth, 
                        config.screenHeight 
                    },
                    (Vector2){ 0, 0 },
                    0.0f,
                    WHITE);
                
                DrawText("Game Over!", config.screenWidth / 2 - 100, config.screenHeight / 2 - 50, 40, RED);
                DrawText("Press Enter to restart or Esc to exit", config.screenWidth / 2 - 200, config.screenHeight / 2, 20, BLACK);
                
                EndDrawing();

                if (IsKeyPressed(KEY_ENTER)) {
                    // Reset game variables and state
                    timer = 0.0f;
                    wave = 1;
                    maxEnemies = wave * 5;
                    sub.speed = 200;
                    sub.health = sub.maxHealth;
                    sub.energy = 100.0f;
                    ResetEnemies(enemies, maxEnemies, wave, &config);
                    gameOver = false;
                    currentState = STATE_LEVEL_SELECTION;
                } else if (IsKeyPressed(KEY_ESCAPE)) {
                    CloseWindow(); // Exit the game
                }

                continue;  
            }

            // Update cooldown timer
            heavyBulletTimer -= GetFrameTime();

            if (IsKeyDown(KEY_A)) {
                sub.facingLeft = true;
            }

            if (IsKeyDown(KEY_D)) {
                sub.facingLeft = false;
            }

            // Submarine movement
            if (IsKeyDown(KEY_W) && sub.rect.y > 0) { 
                sub.rect.y -= sub.speed * GetFrameTime();
            }
            if (IsKeyDown(KEY_S) && sub.rect.y + sub.rect.height < config.screenHeight) { 
                sub.rect.y += sub.speed * GetFrameTime();
            }
            if (IsKeyDown(KEY_A) && sub.rect.x > 0) { 
                sub.rect.x -= sub.speed * GetFrameTime();
            }
            if (IsKeyDown(KEY_D) && sub.rect.x + sub.rect.width < config.screenWidth) { 
                sub.rect.x += sub.speed * GetFrameTime();
            }

            // Speed boost logic
            if (IsKeyDown(KEY_D) && IsKeyDown(KEY_LEFT_SHIFT) && sub.rect.x + sub.rect.width < config.screenWidth) {
                if (hasUnlimitedEnergyBuff || sub.energy > 0) {  
                    sub.rect.x += (sub.speed + 5) * GetFrameTime(); 
                    if (!hasUnlimitedEnergyBuff) {
                        sub.energy -= 1.0f;  
                    }
                }
            }

            if (IsKeyDown(KEY_A) && IsKeyDown(KEY_LEFT_SHIFT) && sub.rect.x > 0) {
                if (hasUnlimitedEnergyBuff || sub.energy > 0) {  
                    sub.rect.x -= (sub.speed + 5) * GetFrameTime(); 
                    if (!hasUnlimitedEnergyBuff) {
                        sub.energy -= 1.0f;  
                    }
                }
            }

            // Update hitbox position after movement
            sub.hitbox.x = sub.rect.x + (sub.rect.width - sub.hitbox.width) / 2; 

            // Shooting bullets
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (!bullets[bulletIndex].active) {
                    Vector2 bulletPos = GetBulletPosition(sub, 5);  
                    bullets[bulletIndex].rect.x = bulletPos.x;
                    bullets[bulletIndex].rect.y = bulletPos.y;
                    bullets[bulletIndex].rect.width = 5;
                    bullets[bulletIndex].rect.height = 10;
                    bullets[bulletIndex].active = true;
                    bullets[bulletIndex].damage = 1;  
                    bulletIndex = (bulletIndex + 1) % 10; 
                }
            }

            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && (heavyBulletTimer <= 0 || hasUnlimitedRightClickBuff)) {
                if (!bullets[bulletIndex].active) {
                    Vector2 bulletPos = GetBulletPosition(sub, 100);
                    bullets[bulletIndex].rect.x = bulletPos.x + 25.0f;
                    bullets[bulletIndex].rect.y = bulletPos.y;
                    bullets[bulletIndex].active = true;
                    bullets[bulletIndex].damage = 3;
                    bullets[bulletIndex].rect.width = 100;
                    bullets[bulletIndex].rect.height = 100;
                    currentRightClickFrame = 0;
                    rightClickFrameTime = 0.0f;
                    bulletIndex = (bulletIndex + 1) % 10;
                    
                    // Only apply cooldown if we don't have unlimited right-click
                    if (!hasUnlimitedRightClickBuff) {
                        heavyBulletTimer = config.heavyBulletCooldown;
                    }
                }
            }

            // Update right-click bullet animation frame
            if (bullets[bulletIndex].active) {
                rightClickFrameTime += GetFrameTime();
                if (rightClickFrameTime >= rightClickUpdateTime) {
                    rightClickFrameTime = 0.0f;
                    currentRightClickFrame = (currentRightClickFrame + 1) % totalRightClickFrames; 
                }
            }

            // Draw the right-click bullet with the current texture
            for (int i = 0; i < 10; i++) {  
                if (bullets[i].active && bullets[i].rect.width > 5) {  
                    DrawTexturePro(rocketTextures[currentRightClickFrame],
                        (Rectangle){ 0, 0, rocketTextures[currentRightClickFrame].width, rocketTextures[currentRightClickFrame].height },
                        (Rectangle){ 
                            bullets[i].rect.x, 
                            bullets[i].rect.y,
                            bullets[i].rect.width * 0.5f,    
                            bullets[i].rect.height * 0.5f    
                        },
                        (Vector2){ 0, 0 },
                        270.0f,    
                        WHITE);
                }
            }

            // Update bullet positions
            for (int i = 0; i < 10; i++) {
                if (bullets[i].active) {
                    bullets[i].rect.y -= 10;
                    if (bullets[i].rect.y < 0) bullets[i].active = false; 
                }
            }

            // Update enemies
            for (int i = 0; i < maxEnemies; i++) {
                if (enemies[i].active) {
                    if (enemies[i].isBoss) {
                        // Boss horizontal movement
                        enemies[i].rect.x += enemies[i].moveDirection * enemies[i].speed;  // Use enemy speed
                        if (enemies[i].rect.x <= 0 || enemies[i].rect.x + enemies[i].rect.width >= config.screenWidth) {
                            enemies[i].moveDirection *= -1;
                        }

                        // Boss shooting
                        enemies[i].shootTimer -= GetFrameTime();
                        if (enemies[i].shootTimer <= 0) {
                            // Find an inactive bullet
                            for (int j = 0; j < config.maxEnemyBullets; j++) {
                                if (!enemyBullets[j].active) {
                                    enemyBullets[j].rect.x = enemies[i].rect.x + enemies[i].rect.width/2;
                                    enemyBullets[j].rect.y = enemies[i].rect.y + enemies[i].rect.height;
                                    enemyBullets[j].rect.width = 10;
                                    enemyBullets[j].rect.height = 10;
                                    enemyBullets[j].active = true;
                                    enemies[i].shootTimer = config.bossShootCooldown;
                                    break;
                                }
                            }
                        }

                    } else if (enemies[i].isShooter) {

                        // Shooter enemy behavior
                        enemies[i].rect.x += enemies[i].moveDirection * 2; 

                        // Reverse direction if it hits the screen edges
                        if (enemies[i].rect.x <= 0 || enemies[i].rect.x + enemies[i].rect.width >= config.screenWidth) {
                            enemies[i].moveDirection *= -1;
                        }

                        // Shooting logic
                        enemies[i].shootTimer -= GetFrameTime();
                        if (enemies[i].shootTimer <= 0) {
                            for (int j = 0; j < config.maxEnemyBullets; j++) {
                                if (!enemyBullets[j].active) {
                                    enemyBullets[j].rect.x = enemies[i].rect.x + enemies[i].rect.width / 2;
                                    enemyBullets[j].rect.y = enemies[i].rect.y + enemies[i].rect.height;
                                    enemyBullets[j].rect.width = 5;
                                    enemyBullets[j].rect.height = 10;
                                    enemyBullets[j].active = true;
                                    enemies[i].shootTimer = config.shooterCooldown;
                                    break;
                                }
                            }
                        }
                    } else {
                        // Modified normal enemy behavior to bounce
                        enemies[i].rect.y += enemies[i].speed * enemies[i].moveDirection;
                        
                        // Bounce at bottom and top of screen
                        if (enemies[i].rect.y + enemies[i].rect.height >= config.screenHeight || 
                            enemies[i].rect.y <= 0) {
                            enemies[i].moveDirection *= -1;  
                        }
                    }

                    // Collision checks with player
                    if (CheckCollisionRecs(sub.hitbox, enemies[i].rect)) {
                        if (wave == 5) {
                            sub.health = 0;  
                        } else {
                            sub.health -= 20;  
                        }
                        if (wave != 5) {  
                            enemies[i].active = false;
                        }
                    }

                    // Bullet collision detect
                    for (int j = 0; j < 10; j++) {
                        if (bullets[j].active && CheckCollisionRecs(bullets[j].rect, enemies[i].rect)) {
                            if (bullets[j].rect.width <= 5) {
                                bullets[j].active = false;  
                            } else if (bullets[j].rect.width == 100) {  
                                bullets[j].active = false;  
                            }
                            
                            enemies[i].health -= bullets[j].damage;
                            if (enemies[i].health <= 0) {
                                if (wave == 5 && enemies[i].isBoss) {
                                    victory = true;
                                }
                                enemies[i].active = false;
                                score += 10;
                                sub.energy += 25;
                                if (sub.energy > 100.0f) {  
                                    sub.energy = 100.0f;
                                }

                                // Add lifesteal effect
                                if (hasLifestealBuff) {
                                    sub.health += 10;
                                    if (sub.health > sub.maxHealth) {
                                        sub.health = sub.maxHealth;
                                    }
                                }
                                
                                continue; 
                            }
                        }
                    }
                }
            }

            // Update enemy bullets
            for (int i = 0; i < config.maxEnemyBullets; i++) {
                if (enemyBullets[i].active) {
                    enemyBullets[i].rect.y += 5;  
                    
                    // Check if bullet is off screen
                    if (enemyBullets[i].rect.y > config.screenHeight) {
                        enemyBullets[i].active = false;
                    }
                    
                    // Check collision with player
                    if (CheckCollisionRecs(enemyBullets[i].rect, sub.hitbox)) {
                        sub.health -= 10; 
                        enemyBullets[i].active = false;
                    }
                }
            }

            bool allDefeated = true;
            for (int i = 0; i < maxEnemies; i++) {
                if (enemies[i].active) {
                    allDefeated = false;
                    break;
                }
            }

            // Only proceed to next wave if all enemies are defeated
            if (allDefeated) {
                if (wave == 2) {  // First buff selection after wave 2
                    currentState = STATE_BUFF_SELECTION;
                } else if (wave == 4) {  // Second buff selection before boss (wave 5)
                    currentState = STATE_BUFF_SELECTION_2;  // New state for second buff selection
                } else if (wave < config.maxWaves) {
                    wave++;
                    maxEnemies = (wave == 5) ? 1 : wave * 5;  
                    sub.speed += 1;
                    ResetEnemies(enemies, maxEnemies, wave, &config);
                }
            }

            // Game over condition
            if (sub.health <= 0) {
                gameOver = true;
            }

            // Update animation frame
            sub.frameTime += GetFrameTime();
            if (sub.frameTime >= config.frameSpeed) {
                sub.frameTime = 0.0f;
                sub.currentFrame++;
                if (sub.currentFrame >= config.frameCount) sub.currentFrame = 0;
            }

            // Draw submarine
            if (!gameOver && !victory) {  
                DrawTexturePro(submarineSheet,
                    (Rectangle){
                        0,                                     
                        0,                                     
                        sub.facingLeft ? -submarineSheet.width : submarineSheet.width,  
                        submarineSheet.height                 
                    },
                    (Rectangle){
                        sub.rect.x,
                        sub.rect.y,
                        sub.rect.width,
                        sub.rect.height
                    },
                    (Vector2){ 0, 0 },
                    0.0f,
                    WHITE);
            }

            // Draw bullets
            for (int i = 0; i < 10; i++) {
                if (bullets[i].active) {
                    if (bullets[i].rect.width > 5) {  
                        DrawTexturePro(rocketTexture1,
                            (Rectangle){ 0, 0, rocketTexture1.width, rocketTexture1.height },
                            (Rectangle){ 
                                bullets[i].rect.x, 
                                bullets[i].rect.y,
                                bullets[i].rect.width * 0.5f,    
                                bullets[i].rect.height * 0.5f    
                            },
                            (Vector2){ 0, 0 },
                            270.0f,   
                            WHITE);
                    } else {
                        // Normal bullet
                        DrawRectangleRec(bullets[i].rect, RED);
                    }
                }
            }

            // Draw enemies
            for (int i = 0; i < maxEnemies; i++) {
                if (enemies[i].active) {
                    Texture2D currentTexture;
                    
                    // Select texture based on movement direction and distance
                    if (enemies[i].isBoss || enemies[i].isShooter) {
                        // Use only front textures for boss and shooter enemies
                        if (enemies[i].distanceMoved < 10) {
                            currentTexture = normalEnemyFront;
                        } else if (enemies[i].distanceMoved < 20) {
                            currentTexture = normalEnemy2Front;
                        } else {
                            currentTexture = normalEnemy3Front;
                        }
                    } else {
                        // Use both front and back textures for normal enemies
                        if (enemies[i].moveDirection > 0) {  // Moving down
                            if (enemies[i].distanceMoved < 10) {
                                currentTexture = normalEnemyFront;
                            } else if (enemies[i].distanceMoved < 20) {
                                currentTexture = normalEnemy2Front;
                            } else {
                                currentTexture = normalEnemy3Front;
                            }
                        } else {  // Moving up
                            if (enemies[i].distanceMoved < 10) {
                                currentTexture = normalEnemyBack;
                            } else if (enemies[i].distanceMoved < 20) {
                                currentTexture = normalEnemy2Back;
                            } else {
                                currentTexture = normalEnemy3Back;
                            }
                        }
                    }

                    // Reset distance after completing animation cycle
                    if (enemies[i].distanceMoved >= 30) {
                        enemies[i].distanceMoved = 0;
                    }

                    DrawTexturePro(
                        currentTexture,
                        (Rectangle){ 
                            0, 
                            0, 
                            currentTexture.width,
                            currentTexture.height 
                        },
                        enemies[i].rect,
                        (Vector2){ 0, 0 },
                        0.0f,
                        WHITE
                    );

                    // Update distance moved
                    enemies[i].distanceMoved += fabsf(enemies[i].speed * enemies[i].moveDirection);

                    // Draw health bar for boss
                    if (enemies[i].isBoss) {
                        float healthBarWidth = 20;
                        float healthBarHeight = 20;
                        float healthBarX = enemies[i].rect.x + (enemies[i].rect.width - healthBarWidth) / 2;
                        float healthBarY = enemies[i].rect.y - healthBarHeight - 5;

                        DrawRectangle(healthBarX, healthBarY, healthBarWidth, healthBarHeight, DARKGRAY);
                        DrawRectangle(healthBarX, healthBarY, healthBarWidth * (enemies[i].health / enemies[i].maxHealth), healthBarHeight, GREEN);
                    }
                }
            }

            // Draw health bar
            DrawRectangle(10, 10, sub.health * 2, 20, GREEN);
            DrawRectangleLines(10, 10, sub.maxHealth * 2, 20, BLACK);
            DrawText(TextFormat("Health: %d", sub.health), 15, 10, 20, WHITE);

            // Draw energy bar
            DrawRectangle(10, 35, sub.energy * 2, 20, BLUE);  
            DrawRectangleLines(10, 35, 200, 20, BLACK);  
            DrawText(TextFormat("Energy", sub.energy), 15, 35, 20, WHITE);  

            // Draw the timer
            DrawText(TextFormat("Time: %.1f", timer), config.screenWidth - 120, 70, 20, WHITE);

            // Wave and score
            DrawText(TextFormat("Wave: %d/%d", wave, 5), config.screenWidth - 120, 10, 20, WHITE);
            DrawText(TextFormat("Score: %d", score), config.screenWidth - 120, 40, 20, WHITE);

            // Enemy bullets
            for (int i = 0; i < config.maxEnemyBullets; i++) {
                if (enemyBullets[i].active) {
                    DrawRectangleRec(enemyBullets[i].rect, ORANGE); 
                }
            }

            printf("Submarine Speed: %f, Position: (%f, %f)\n", sub.speed, sub.rect.x, sub.rect.y);

            EndDrawing();
        } else if (currentState == STATE_BUFF_SELECTION) {
            BeginDrawing();
            DrawTexturePro(backgroundMenuTexture2, 
                (Rectangle){ 
                    backgroundScrollX, 
                    0, 
                    config.screenWidth, 
                    backgroundMenuTexture2.height 
                },
                (Rectangle){ 
                    0, 
                    0, 
                    config.screenWidth, 
                    config.screenHeight 
                },
                (Vector2){ 0, 0 },
                0.0f,
                WHITE);

            DrawText("Choose Your Buff:", config.screenWidth / 2 - 100, config.screenHeight / 2 - 50, 30, GRAY);

            // Define button rectangles for buffs
            Rectangle lifestealButton = { config.screenWidth / 2 - 100, config.screenHeight / 2, 200, 50 };
            Rectangle unlimitedRightClickButton = { config.screenWidth / 2 - 100, config.screenHeight / 2 + 70, 200, 50 };

            // Check for mouse position
            Vector2 mousePos = GetMousePosition();

            // Draw Lifesteal button with hover effect
            if (CheckCollisionPointRec(mousePos, lifestealButton)) {
                DrawTexture(menuButtonPressedTexture, lifestealButton.x, lifestealButton.y, WHITE);
            } else {
                DrawTexture(menuButtonTexture, lifestealButton.x, lifestealButton.y, WHITE);
            }
            DrawText("Lifesteal (+10 HP/Kill)", lifestealButton.x + 20, lifestealButton.y + 10, 13, WHITE);

            // Draw Unlimited Right Click button with hover effect
            if (CheckCollisionPointRec(mousePos, unlimitedRightClickButton)) {
                DrawTexture(menuButtonPressedTexture, unlimitedRightClickButton.x, unlimitedRightClickButton.y, WHITE);
            } else {
                DrawTexture(menuButtonTexture, unlimitedRightClickButton.x, unlimitedRightClickButton.y, WHITE);
            }
            DrawText("Unlimited Special Attack", unlimitedRightClickButton.x + 20, unlimitedRightClickButton.y + 10, 13, WHITE);

            EndDrawing();

            // Handle buff selection input
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mousePos, lifestealButton)) {
                    hasLifestealBuff = true;
                    wave++;
                    maxEnemies = wave * 5;
                    ResetEnemies(enemies, maxEnemies, wave, &config);
                    currentState = STATE_PLAYING;
                } else if (CheckCollisionPointRec(mousePos, unlimitedRightClickButton)) {
                    hasUnlimitedRightClickBuff = true;
                    wave++;
                    maxEnemies = wave * 5;
                    ResetEnemies(enemies, maxEnemies, wave, &config);
                    currentState = STATE_PLAYING;
                }
            }
        } else if (currentState == STATE_BUFF_SELECTION_2) {
            BeginDrawing();
            DrawTexturePro(backgroundMenuTexture2,  
                (Rectangle){ 
                    backgroundScrollX, 
                    0, 
                    config.screenWidth, 
                    backgroundMenuTexture2.height 
                },
                (Rectangle){ 
                    0, 
                    0, 
                    config.screenWidth, 
                    config.screenHeight 
                },
                (Vector2){ 0, 0 },
                0.0f,
                WHITE);

            DrawText("Choose Your Pre-Boss Buff:", config.screenWidth / 2 - 150, config.screenHeight / 2 - 50, 30, GRAY);

            // Define button rectangles for buffs
            Rectangle unlimitedEnergyButton = { config.screenWidth / 2 - 100, config.screenHeight / 2, 200, 50 };
            Rectangle fullHealthButton = { config.screenWidth / 2 - 100, config.screenHeight / 2 + 70, 200, 50 };

            // Check for mouse position
            Vector2 mousePos = GetMousePosition();

            // Draw Unlimited Energy button with hover effect
            if (CheckCollisionPointRec(mousePos, unlimitedEnergyButton)) {
                DrawTexture(menuButtonPressedTexture, unlimitedEnergyButton.x, unlimitedEnergyButton.y, WHITE);
            } else {
                DrawTexture(menuButtonTexture, unlimitedEnergyButton.x, unlimitedEnergyButton.y, WHITE);
            }
            DrawText("Unlimited Energy", unlimitedEnergyButton.x + 20, unlimitedEnergyButton.y + 10, 18, WHITE);

            // Draw Full Health button with hover effect
            if (CheckCollisionPointRec(mousePos, fullHealthButton)) {
                DrawTexture(menuButtonPressedTexture, fullHealthButton.x, fullHealthButton.y, WHITE);
            } else {
                DrawTexture(menuButtonTexture, fullHealthButton.x, fullHealthButton.y, WHITE);
            }
            DrawText("Full Health Restore", fullHealthButton.x + 20, fullHealthButton.y + 10, 18, WHITE);

            EndDrawing();

            // Handle buff selection input
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mousePos, unlimitedEnergyButton)) {
                    sub.energy = 100.0f;  // Set energy to max
                    hasUnlimitedEnergyBuff = true;  // Add this variable at the top with other buff variables
                    wave++;
                    maxEnemies = wave * 5;
                    ResetEnemies(enemies, maxEnemies, wave, &config);
                    currentState = STATE_PLAYING;
                } else if (CheckCollisionPointRec(mousePos, fullHealthButton)) {
                    sub.health = sub.maxHealth;  // Restore full health
                    wave++;
                    maxEnemies = wave * 5;
                    ResetEnemies(enemies, maxEnemies, wave, &config);
                    currentState = STATE_PLAYING;
                }
            }
        } else if (currentState == STATE_OPTIONS) {
            BeginDrawing();
            
            // Update scroll position for options background
            optionsScrollX += optionsScrollSpeed;
            if (optionsScrollX >= backgroundMenuTexture.width) {
                optionsScrollX = 0;  
            }

            // Draw the scrolling background
            DrawTexturePro(backgroundMenuTexture,
                (Rectangle){ optionsScrollX, 0, config.screenWidth, backgroundMenuTexture.height },
                (Rectangle){ 0, 0, config.screenWidth, config.screenHeight },
                (Vector2){ 0, 0 },
                0.0f,
                WHITE);

            // Draw the settings title
            DrawText("Settings", config.screenWidth / 2 - 50, 50, 30, WHITE);

            DrawText("Controls", 100, 100, 20, WHITE);
            DrawText("Move Left: A", 100, 130, 20, WHITE);
            DrawText("Move Right: D", 100, 150, 20, WHITE);
            DrawText("Use Speed Boost: Shift + A / D", 100, 170, 20, WHITE);
            DrawText("Shoot: Left Mouse Button", 100, 190, 20, WHITE);
            DrawText("Use Special Attack: Right Mouse Button", 100, 210, 20, WHITE);
            DrawText("Exit Game: Esc", 100, 230, 20, WHITE);    

            // Volume Slider
            DrawText("Music Volume:", 100, 270, 20, WHITE);
            Rectangle volumeSlider = { 100, 300, 200, 20 }; 
            DrawRectangleRec(volumeSlider, DARKGRAY);  

            //current volume level
            Rectangle volumeBar = { 100, 300, 200 * musicVolume, 20 };  
            DrawRectangleRec(volumeBar, GREEN);  

            // check mouse position on the current volume
            Vector2 mousePos = GetMousePosition();
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, volumeSlider)) {
                musicVolume = (mousePos.x - volumeSlider.x) / volumeSlider.width;  
                if (musicVolume < 0) musicVolume = 0;  
                if (musicVolume > 1) musicVolume = 1;  
                SetMusicVolume(backgroundMusic, musicVolume);  
            }

            // Draw the back button
            Rectangle backButton = {10, 10, 60, 55};  
            if (CheckCollisionPointRec(mousePos, backButton)) {
                DrawTexture(backButtonPressedTexture, backButton.x, backButton.y, WHITE); 
            } else {
                DrawTexture(backButtonTexture, backButton.x, backButton.y, WHITE); 
            }

            DrawText("←", backButton.x + backButton.width / 2 - 10, backButton.y + backButton.height / 2 - 10, 20, WHITE); 

            // Check for button clicks to Exit
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePos, backButton)) {
                currentState = STATE_MENU; 
            }

            EndDrawing();
        } else if (currentState == STATE_VICTORY) {
            BeginDrawing();
            // Change the background to match the game over background
            DrawTexturePro(backgroundMenuTexture,  // Use the same texture as the game over screen
                (Rectangle){ 
                    backgroundScrollX, 
                    0, 
                    config.screenWidth, 
                    backgroundMenuTexture.height 
                },
                (Rectangle){ 
                    0, 
                    0, 
                    config.screenWidth, 
                    config.screenHeight 
                },
                (Vector2){ 0, 0 },
                0.0f,
                WHITE);
            
            // Display victory text and time
            DrawText("Victory!", config.screenWidth / 2 - 100, config.screenHeight / 2 - 50, 40, WHITE);
            DrawText(TextFormat("Time: %.1f", timer), config.screenWidth / 2 - 80, config.screenHeight / 2, 20, WHITE);
            DrawText("Press Enter to restart or Esc to exit", config.screenWidth / 2 - 200, config.screenHeight / 2 + 30, 20, BLACK);
            
            EndDrawing();

            if (IsKeyPressed(KEY_ENTER)) {
                // Write the times to file before resetting
                WriteLowestTimes(lowestTimes, 3);
                
                // Reset all game variables
                timer = 0.0f;
                wave = 1;
                maxEnemies = wave * 5;
                sub.speed = 200;
                sub.health = sub.maxHealth;
                sub.energy = 100.0f;
                ResetEnemies(enemies, maxEnemies, wave, &config);
                victory = false;
                gameOver = false; 
                currentState = STATE_LEVEL_SELECTION;
            } else if (IsKeyPressed(KEY_ESCAPE)) {
                CloseWindow(); // Exit the game
            }
        }
    }

    UnloadTexture(submarineSheet);
    UnloadTexture(rocketTexture1);
    UnloadTexture(startMenuTexture);
    UnloadTexture(backgroundMenuTexture);
    UnloadTexture(backgroundMenuTexture2);
    UnloadTexture(normalEnemyFront);
    UnloadTexture(normalEnemyBack);
    UnloadTexture(normalEnemy2Front);
    UnloadTexture(normalEnemy3Front);
    UnloadTexture(normalEnemy2Back);
    UnloadTexture(normalEnemy3Back);
    UnloadTexture(bossTexture);
    UnloadTexture(leftClickAnimationTexture);
    UnloadFont(config.customFont);
    UnloadTexture(waterTexture);
    CloseWindow();
    return 0;
}


void ResetEnemies(Enemy enemies[], int enemyCount, int wave, const GameConfig* config) {
    if (wave == 5) {
        enemies[0].rect.x = config->screenWidth / 2 - 100;
        enemies[0].rect.y = 50;
        enemies[0].rect.width = 200;
        enemies[0].rect.height = 200;
        enemies[0].speed = 2;
        enemies[0].maxHealth = 50;
        enemies[0].health = enemies[0].maxHealth;
        enemies[0].active = true;
        enemies[0].isBoss = true;
        enemies[0].shootTimer = 0;
        enemies[0].spawnTimer = 0;
        enemies[0].moveDirection = 1;  
        enemies[0].isShooter = false;  


        for (int i = 1; i < config->maxEnemies; i++) {
            enemies[i].active = false;
            enemies[i].isBoss = false;
        }
        return;
    }


    for (int i = 0; i < enemyCount && i < config->maxEnemies; i++) {
        enemies[i].rect.x = GetRandomValue(0, config->screenWidth - 40);
        enemies[i].rect.y = GetRandomValue(50, config->screenHeight/2);  
        enemies[i].rect.width = 64;   
        enemies[i].rect.height = 64;  
        enemies[i].speed = 2;
        enemies[i].maxHealth = wave;
        enemies[i].health = enemies[i].maxHealth;
        enemies[i].active = true;
        enemies[i].isBoss = false;
        enemies[i].moveDirection = 1;  
        

        if (wave >= 3 && GetRandomValue(0, 4) == 0) {
            enemies[i].isShooter = true;
            enemies[i].shootTimer = 0;
            enemies[i].moveDirection = GetRandomValue(0, 1) * 2 - 1;  
        } else {
            enemies[i].isShooter = false;
        }
        enemies[i].distanceMoved = 0;
    }

    printf("Wave %d: Spawned %d enemies\n", wave, enemyCount);
}

bool CheckBossWaveComplete(Enemy enemies[], const GameConfig* config) {
    if (enemies[0].active) return false;
    
    for (int i = 1; i < config->maxEnemies; i++) {
        if (enemies[i].active) return false;
    }
    
    return true;
}

bool CheckCollisionWithWalls(float newX, float subY, Rectangle subRect, const GameConfig* config) {
    // Check for collision with walls using the passed parameters
    if (newX < 0 || newX + subRect.width > config->screenWidth) {
        return true; 
    }
    return false; 
}
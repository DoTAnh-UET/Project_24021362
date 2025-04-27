#include <bits/stdc++.h>
#include "src/include/SDL2/SDL.h"
#include "src/include/SDL2/SDL_image.h"
#include "src/include/SDL2/SDL_timer.h"
#include "src/include/SDL2/SDL_ttf.h"
#include "src/include/SDL2/SDL_mixer.h"
#include <fstream>

#undef main
using namespace std;

// Struct cho vi tri nhan vat
struct Position {
    int x, y;
};

// Struct cho cau lua
struct Fireball {
    Position pos;        // Position of the fireball
    float velocityX, velocityY; // Velocity along x and y axes
    bool active;         // Active state
    bool scored;         // Whether scored or not
};

// Struct cho sao
struct Star {
    Position pos;        // Position of the star
    float velocityY;     // Falling velocity along y-axis
    bool active;         // Active state
};

// Struct Button cho cac nut bam
struct Button {
    SDL_Rect rect;       // Position and size of the button
    string text;         // Text displayed on the button
    bool hovered;        // Hover state
};

// Cac trang thai game
enum GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
};

// Ham lay diem cao tu file
int loadHighscore() {
    ifstream file("highscore.txt");
    int highscore = 0;
    if (file.is_open()) {
        file >> highscore;
        file.close();
    }
    return highscore;
}

// Ham luu diem cao nhat
void saveHighscore(int highscore) {
    ofstream file("highscore.txt");
    if (file.is_open()) {
        file << highscore;
        file.close();
    } else {
        cerr << "Cannot open file to save high score!" << endl;
    }
}

// Ham tao nut bam
void renderButton(SDL_Renderer* renderer, TTF_Font* font, Button& button, SDL_Color color) {
    SDL_Rect buttonRect = button.rect;
    if (button.hovered) {
        buttonRect.w *= 1.1; // Tang 10% khi di chuot toi
        buttonRect.h *= 1.1;
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &buttonRect);

    SDL_Surface* surface = TTF_RenderText_Solid(font, button.text.c_str(), {255, 255, 255});
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect textRect = {buttonRect.x + (buttonRect.w - surface->w) / 2, buttonRect.y + (buttonRect.h - surface->h) / 2, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &textRect);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// Menu Events
void handleMenuEvents(SDL_Event& event, vector<Button>& menuButtons, GameState& gameState, bool& running, Mix_Music* backgroundMusic, Mix_Chunk* jumpSound, Mix_Chunk* collectStarSound, Mix_Chunk* hitSound, Mix_Chunk* pauseSound, bool& soundOn, bool& showInstructionsPanel, Button& closeInstructionsButton) {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    // Cac nut trong Menu
    for (auto& button : menuButtons) {
        button.hovered = (mouseX >= button.rect.x && mouseX <= button.rect.x + button.rect.w &&
                          mouseY >= button.rect.y && mouseY <= button.rect.y + button.rect.h);
        if (event.type == SDL_MOUSEBUTTONDOWN && button.hovered) {
            if (button.text == "Play") {
                gameState = PLAYING;
            } else if (button.text.find("Sound") != string::npos) {
                soundOn = !soundOn;
                if (soundOn) {
                    Mix_PlayMusic(backgroundMusic, -1); // Play music
                    button.text = "Sound: On";
                } else {
                    Mix_PauseMusic(); // Pause music
                    Mix_HaltChannel(-1); // Stop all sound effects
                    button.text = "Sound: Off";
                }
            } else if (button.text == "Exit") {
                running = false;
            } else if (button.text == "Instructions") {
                showInstructionsPanel = true;
            }
        }
    }

    // Nut close Instructions
    if (showInstructionsPanel) {
        closeInstructionsButton.hovered = (mouseX >= closeInstructionsButton.rect.x && mouseX <= closeInstructionsButton.rect.x + closeInstructionsButton.rect.w &&
                                           mouseY >= closeInstructionsButton.rect.y && mouseY <= closeInstructionsButton.rect.y + closeInstructionsButton.rect.h);
        if (event.type == SDL_MOUSEBUTTONDOWN && closeInstructionsButton.hovered) {
            showInstructionsPanel = false;
        }
    }
}

// Paused Events
void handlePausedEvents(SDL_Event& event, vector<Button>& pausedButtons, GameState& gameState, bool& running, Mix_Chunk* pauseSound, bool soundOn, Position& playerPos, vector<Fireball>& fireballs, vector<Star>& stars, int& spawnTimer, int& starSpawnTimer, int& countdownTime, bool& razzyMode, int& razzyModeMessageTimer, float& fireballSpeed, int& spawnInterval, int& lives, int& score, bool& isJumping, bool& isMovingRight, float& horizontalVelocity, float& verticalVelocity) {
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);

    for (auto& button : pausedButtons) {
        button.hovered = (mouseX >= button.rect.x && mouseX <= button.rect.x + button.rect.w &&
                          mouseY >= button.rect.y && mouseY <= button.rect.y + button.rect.h);
        if (event.type == SDL_MOUSEBUTTONDOWN && button.hovered) {
            if (button.text == "Resume") {
                gameState = PLAYING;
                if (soundOn && pauseSound) {
                    Mix_PlayChannel(-1, pauseSound, 0);
                }
            } else if (button.text == "Quit") {
                running = false;
            } else if (button.text == "Back to Menu") {
                gameState = MENU;
                playerPos = {400, 400};
                isJumping = false;
                isMovingRight = true;
                horizontalVelocity = 0.0f;
                verticalVelocity = 0.0f;
                fireballs.clear();
                stars.clear();
                spawnTimer = 0;
                starSpawnTimer = 0;
                countdownTime = 20 * 60;
                razzyMode = false;
                razzyModeMessageTimer = 0;
                fireballSpeed = 3.0f;
                spawnInterval = 120;
                lives = 5;
                score = 0;
            }
        }
    }
}

// Render Menu
void renderMenu(SDL_Renderer* renderer, TTF_Font* font, SDL_Texture* back0_texture, vector<Button>& menuButtons, bool showInstructionsPanel, Button& closeInstructionsButton) {
    if (back0_texture) {
        SDL_RenderCopy(renderer, back0_texture, NULL, NULL);
    }

    for (auto& button : menuButtons) {
        renderButton(renderer, font, button, {100, 100, 100, 255});
    }

    // Sau khi thay duoc Menu Instructions
    if (showInstructionsPanel) {
        // Dieu chinh Transparency cua Menu Instructions
        SDL_Rect panelRect = {212, 88, 600, 400}; // Centered: (1024-600)/2 = 212, (576-400)/2 = 88
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 200); // Semi-transparent gray
        SDL_RenderFillRect(renderer, &panelRect);

        // Tao Instructions 
        SDL_Color white = {255, 255, 255, 255};
        vector<string> instructions = {
            "Game Instructions:",
            "Avoid fireballs, collect stars!",
            "A/D: Move left/right",
            "W: Jump",
            "P: Pause game"
        };

        int yPos = 120; // Hien thi text cho Menu Instruction
        for (const auto& line : instructions) {
            SDL_Surface* instructionsSurface = TTF_RenderText_Blended(font, line.c_str(), white);
            if (instructionsSurface) {
                SDL_Texture* instructionsTexture = SDL_CreateTextureFromSurface(renderer, instructionsSurface);
                if (instructionsTexture) {
                    SDL_Rect instructionsRect = {(1024 - instructionsSurface->w) / 2, yPos, instructionsSurface->w, instructionsSurface->h};
                    SDL_RenderCopy(renderer, instructionsTexture, NULL, &instructionsRect);
                    SDL_DestroyTexture(instructionsTexture);
                }
                SDL_FreeSurface(instructionsSurface);
            }
            yPos += 30; // Xuong dong ke tiep
        }

        // Render nut close Instruction
        renderButton(renderer, font, closeInstructionsButton, {100, 100, 100, 255});
    }
}

// Render man hinh pause game
void renderPaused(SDL_Renderer* renderer, TTF_Font* font, vector<Button>& pausedButtons, int score) {
    SDL_Color white = {255, 255, 255, 255};

    // Tao text cho Menu pause game
    SDL_Surface* pausedSurface = TTF_RenderText_Solid(font, "Paused", white);
    if (pausedSurface) {
        SDL_Texture* pausedTexture = SDL_CreateTextureFromSurface(renderer, pausedSurface);
        if (pausedTexture) {
            SDL_Rect pausedRect = {(1024 - pausedSurface->w) / 2, (576 - pausedSurface->h) / 2 - 60, pausedSurface->w, pausedSurface->h};
            SDL_RenderCopy(renderer, pausedTexture, NULL, &pausedRect);
            SDL_DestroyTexture(pausedTexture);
        }
        SDL_FreeSurface(pausedSurface);
    }

    // Diem hien tai khi pause game
    string pauseScoreText = "Current Score: " + to_string(score);
    SDL_Surface* pauseScoreSurface = TTF_RenderText_Blended(font, pauseScoreText.c_str(), white);
    if (pauseScoreSurface) {
        SDL_Texture* pauseScoreTexture = SDL_CreateTextureFromSurface(renderer, pauseScoreSurface);
        if (pauseScoreTexture) {
            SDL_Rect pauseScoreRect = {(1024 - pauseScoreSurface->w) / 2, (576 - pauseScoreSurface->h) / 2 - 20, pauseScoreSurface->w, pauseScoreSurface->h};
            SDL_RenderCopy(renderer, pauseScoreTexture, NULL, &pauseScoreRect);
            SDL_DestroyTexture(pauseScoreTexture);
        }
        SDL_FreeSurface(pauseScoreSurface);
    }

    // Render Menu pause game
    for (auto& button : pausedButtons) {
        renderButton(renderer, font, button, {100, 100, 100, 255});
    }
}

int main(int argc, char* argv[]) {
    // Khoi tao SDL voi VIDEO va AUDIO
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        cerr << "SDL cannot initialize! SDL_Error: " << SDL_GetError() << endl;
        return 1;
    }

    // Khoi tao SDL_image
    if (!(IMG_Init(IMG_INIT_PNG))) {
        cerr << "SDL_image cannot initialize! IMG_Error: " << IMG_GetError() << endl;
        SDL_Quit();
        return 1;
    }

    // Khoi tao SDL_ttf
    if (TTF_Init() < 0) {
        cerr << "SDL_ttf cannot initialize! TTF_Error: " << TTF_GetError() << endl;
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Khoi tao SDL_Mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        cerr << "SDL_mixer cannot initialize! SDL_Error: " << Mix_GetError() << endl;
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Tao cua so game
    SDL_Window* window = SDL_CreateWindow("DINO RUN",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          1024, 576, SDL_WINDOW_SHOWN);
    if (!window) {
        cerr << "Cannot create window! SDL_Error: " << SDL_GetError() << endl;
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Tao Renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        cerr << "Cannot create renderer! SDL_Error: " << SDL_GetError() << endl;
        SDL_DestroyWindow(window);
        Mix_CloseAudio();
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    // Mo font
    TTF_Font* font = TTF_OpenFont("font.ttf", 20);
    if (!font) {
        cerr << "Cannot load font! TTF_Error: " << TTF_GetError() << endl;
    }

    // Mo Texture cho cac elements
    SDL_Texture* back0_texture = IMG_LoadTexture(renderer, "images/back0.png");
    if (!back0_texture) {
        cerr << "Cannot load back0.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* back1_texture = IMG_LoadTexture(renderer, "images/back1.png");
    if (!back1_texture) {
        cerr << "Cannot load back1.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* player_jump_texture = IMG_LoadTexture(renderer, "images/jump.png");
    if (!player_jump_texture) {
        cerr << "Cannot load jump.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* player_run_texture = IMG_LoadTexture(renderer, "images/run.png");
    if (!player_run_texture) {
        cerr << "Cannot load run.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* player_stop_texture = IMG_LoadTexture(renderer, "images/stop.png");
    if (!player_stop_texture) {
        cerr << "Cannot load stop.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* fireball_texture = IMG_LoadTexture(renderer, "images/fireball.png");
    if (!fireball_texture) {
        cerr << "Cannot load fireball.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* heart_texture = IMG_LoadTexture(renderer, "images/tim.png");
    if (!heart_texture) {
        cerr << "Cannot load tim.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* star_texture = IMG_LoadTexture(renderer, "images/sao.png");
    if (!star_texture) {
        cerr << "Cannot load sao.png! SDL_Error: " << SDL_GetError() << endl;
    }

    // Mo am thanh game
    Mix_Music* backgroundMusic = Mix_LoadMUS("audio/background_music.mp3");
    if (!backgroundMusic) {
        cerr << "Cannot load background music! SDL_Error: " << Mix_GetError() << endl;
    }

    Mix_Chunk* jumpSound = Mix_LoadWAV("audio/jump.wav");
    if (!jumpSound) {
        cerr << "Cannot load jump.wav! SDL_Error: " << SDL_GetError() << endl;
    }

    Mix_Chunk* collectStarSound = Mix_LoadWAV("audio/collect_star.wav");
    if (!collectStarSound) {
        cerr << "Cannot load collect_star.wav! SDL_Error: " << Mix_GetError() << endl;
    }

    Mix_Chunk* hitSound = Mix_LoadWAV("audio/hit.wav");
    if (!hitSound) {
        cerr << "Cannot load hit.wav! SDL_Error: " << SDL_GetError() << endl;
    }

    Mix_Chunk* pauseSound = Mix_LoadWAV("audio/pause.wav");
    if (!pauseSound) {
        cerr << "Cannot load pause.wav! SDL_Error: " << SDL_GetError() << endl;
    }

    // Bien game
    bool running = false;           // Bien exit game
    GameState gameState = MENU;     // Trang thai ban dau la MENU
    int razzyModeMessageTimer = 0;  // Thoi gian thong bao Razzy Mode
    const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL); //Trang thai ban phim
    const int frameDelay = 1000 / 60; // Limit FPS (60 FPS)

    // Vi tri nhan vat 
    Position playerPos = {400, 400}; // Vi tri ban dau
    bool isJumping = false;          // Trang thai nhay
    bool isMovingRight = true;       // Huong di chuyen
    float horizontalVelocity = 0.0f; // Van toc di chuyen co huong
    float verticalVelocity = 0.0f;   // Van toc nhay
    const float moveSpeed = 5.0f;    // Toc do di chuyen
    const float jumpForce = -13.0f;  // Luc nhay
    const float gravity = 0.5f;      // Trong luc

    // Kich thuoc trai tim
    const int heartWidth = 32;
    const int heartHeight = 32;

    //  Vector Fireball (Cau lua)
    vector<Fireball> fireballs;      
    const int fireballWidth = 50;    // Chieu rong cau lua
    const int fireballHeight = 50;   // Chieu cao cau lua
    float fireballSpeed = 3.0f;      // Toc do cau lua roi
    int spawnTimer = 0;              // Dem nguoc spawn cau lua
    int spawnInterval = 120;         // Khoang thoi gian spawn cau lua

    // Vector Star (Sao)
    vector<Star> stars;              
    const int starWidth = 64;        // Chieu rong sao
    const int starHeight = 64;       // Chieu cao sao
    const float starFallSpeed = 2.0f;// Toc do sao roi
    const int starSpawnInterval = 300; // Khoang thoi gian spawn sao
    int starSpawnTimer = 0;          // Dem nguoc spawn sao

    // Dem nguoc 20s den Razzy Mode
    int countdownTime = 20 * 60;     
    bool razzyMode = false;          

    // Mang
    int lives = 5;

    // Diem
    int score = 0;
    int highscore = loadHighscore();

    // Mau sac
    SDL_Color white = {255, 255, 255, 255};

    // Bat am thanh
    bool soundOn = true;

    // Instruction
    bool showInstructionsPanel = false;

    // Vector Button la cac nut bam de choi
    vector<Button> menuButtons = {
        {{112, 200, 200, 50}, "Play", false},
        {{112, 300, 200, 50}, "Sound: On", false},
        {{112, 400, 200, 50}, "Exit", false},
        {{112, 500, 200, 50}, "Instructions", false}
    };

    // Dong (Close) Instruction
    Button closeInstructionsButton = {{412, 430, 200, 50}, "Close", false}; 

    // Vector nut Pause
    vector<Button> pausedButtons = {
        {{412, 250, 200, 50}, "Resume", false},
        {{412, 350, 200, 50}, "Quit", false},
        {{412, 450, 200, 50}, "Back to Menu", false}
    };

    // Restart sau Game Over
    Button restartButton = {{412, 350, 200, 50}, "Restart", false};

    // Bat nhac
    if (soundOn && backgroundMusic) {
        Mix_PlayMusic(backgroundMusic, -1);
    }

    // Main game loop
    running = true;
    SDL_Event e;
    while (running) {
        Uint32 frameStart = SDL_GetTicks();

        // Events Game
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            }
            if (gameState == MENU) {
                handleMenuEvents(e, menuButtons, gameState, running, backgroundMusic, jumpSound, collectStarSound, hitSound, pauseSound, soundOn, showInstructionsPanel, closeInstructionsButton);
            } else if (gameState == PLAYING) {
                if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_w && !isJumping) {
                        isJumping = true;
                        verticalVelocity = jumpForce;
                        if (soundOn && jumpSound) {
                            Mix_PlayChannel(-1, jumpSound, 0);
                        }
                    }
                    if (e.key.keysym.sym == SDLK_p) {
                        gameState = PAUSED;
                        if (soundOn && pauseSound) {
                            Mix_PlayChannel(-1, pauseSound, 0);
                        }
                    }
                }
            } else if (gameState == PAUSED) {
                handlePausedEvents(e, pausedButtons, gameState, running, pauseSound, soundOn, playerPos, fireballs, stars, spawnTimer, starSpawnTimer, countdownTime, razzyMode, razzyModeMessageTimer, fireballSpeed, spawnInterval, lives, score, isJumping, isMovingRight, horizontalVelocity, verticalVelocity);
            } else if (gameState == GAME_OVER) {
                int mouseX, mouseY;
                SDL_GetMouseState(&mouseX, &mouseY);
                restartButton.hovered = (mouseX >= restartButton.rect.x && mouseX <= restartButton.rect.x + restartButton.rect.w &&
                                        mouseY >= restartButton.rect.y && mouseY <= restartButton.rect.y + restartButton.rect.h);
                if (e.type == SDL_MOUSEBUTTONDOWN && restartButton.hovered) {
                    // Reset thuoc tinh game
                    gameState = PLAYING;
                    playerPos = {400, 400};
                    isJumping = false;
                    isMovingRight = true;
                    horizontalVelocity = 0.0f;
                    verticalVelocity = 0.0f;
                    fireballs.clear();
                    stars.clear();
                    spawnTimer = 0;
                    starSpawnTimer = 0;
                    countdownTime = 20 * 60;
                    razzyMode = false;
                    razzyModeMessageTimer = 0;
                    fireballSpeed = 3.0f;
                    spawnInterval = 120;
                    lives = 5;
                    score = 0;
                }
            }
        }

        // Update logic game neu van con choi
        if (gameState == PLAYING) {
            horizontalVelocity = 0.0f;
            if (currentKeyStates[SDL_SCANCODE_A]) {
                horizontalVelocity = -moveSpeed;
                isMovingRight = false;
            }
            if (currentKeyStates[SDL_SCANCODE_D]) {
                horizontalVelocity = moveSpeed;
                isMovingRight = true;
            }

            // Update nhan vat di chuyen
            playerPos.x += static_cast<int>(horizontalVelocity);
            if (playerPos.x < 0) playerPos.x = 0;
            if (playerPos.x > 1024 - 64) playerPos.x = 1024 - 64;

            // Update nhan vat nhay
            if (isJumping) {
                playerPos.y += static_cast<int>(verticalVelocity);
                verticalVelocity += gravity;
            }
            if (playerPos.y >= 400) {
                playerPos.y = 400;
                isJumping = false;
                verticalVelocity = 0.0f;
            }

            // Countdown logic de chuyen sang Razzy Mode
            if (!razzyMode) {
                countdownTime--;
                if (countdownTime <= 0) {
                    razzyMode = true;
                    fireballSpeed = 5.0f;
                    spawnInterval = 60;
                    razzyModeMessageTimer = 180; // Display "Razzy Mode!" for 3 seconds
                }
            } else {
                if (countdownTime > 0) {
                    razzyMode = false;
                    fireballSpeed = 3.0f;
                    spawnInterval = 120;
                }
                countdownTime--;
                if (countdownTime < 0) countdownTime = 0;
            }

            // Tao cau lua
            spawnTimer++;
            if (spawnTimer >= spawnInterval) {
                Fireball fireball;
                fireball.pos.x = rand() % (1024 - fireballWidth);
                fireball.pos.y = -fireballHeight;
                fireball.active = true;
                fireball.scored = false;
                float dx = playerPos.x - fireball.pos.x;
                float dy = playerPos.y - fireball.pos.y;
                float distance = sqrt(dx * dx + dy * dy);
                if (distance > 0) {
                    fireball.velocityX = (dx / distance) * fireballSpeed;
                    fireball.velocityY = (dy / distance) * fireballSpeed;
                } else {
                    fireball.velocityX = 0;
                    fireball.velocityY = fireballSpeed;
                }
                fireballs.push_back(fireball);
                spawnTimer = 0;
            }

            // Update cau lua
            for (int i = 0; i < fireballs.size(); i++) {
                if (fireballs[i].active) {
                    fireballs[i].pos.x += static_cast<int>(fireballs[i].velocityX);
                    fireballs[i].pos.y += static_cast<int>(fireballs[i].velocityY);

                    SDL_Rect fireballRect = {fireballs[i].pos.x, fireballs[i].pos.y, fireballWidth, fireballHeight};
                    SDL_Rect playerRect = {playerPos.x, playerPos.y, 64, 64};
                    if (SDL_HasIntersection(&fireballRect, &playerRect)) {
                        fireballs[i].active = false;
                        lives--;
                        if (soundOn && hitSound) {
                            Mix_PlayChannel(-1, hitSound, 0);
                        }
                        if (lives <= 0) {
                            gameState = GAME_OVER;
                        }
                    }

                    if (fireballs[i].pos.y > 576) {
                        fireballs[i].active = false;
                    }

                    if (!fireballs[i].scored && fireballs[i].pos.y > 576) {
                        score++;
                        fireballs[i].scored = true;
                        if (score > highscore) {
                            highscore = score;
                        }
                    }
                }
            }
            fireballs.erase(
                remove_if(fireballs.begin(), fireballs.end(), [](const Fireball& f) { return !f.active; }),
                fireballs.end()
            );

            // Tao sao  
            starSpawnTimer++;
            if (razzyMode && starSpawnTimer >= starSpawnInterval) {
                Star star;
                star.pos.x = rand() % (1024 - starWidth);
                star.pos.y = -starHeight;
                star.velocityY = starFallSpeed;
                star.active = true;
                stars.push_back(star);
                starSpawnTimer = 0;
            }

            // Update sao (stars)
            for (int i = 0; i < stars.size(); i++) {
                if (stars[i].active) {
                    stars[i].pos.y += static_cast<int>(stars[i].velocityY);

                    SDL_Rect starRect = {stars[i].pos.x, stars[i].pos.y, starWidth, starHeight};
                    SDL_Rect playerRect = {playerPos.x, playerPos.y, 64, 64};
                    if (SDL_HasIntersection(&starRect, &playerRect)) {
                        stars[i].active = false;
                        countdownTime += 5 * 60;
                        if (countdownTime > 20 * 60) countdownTime = 20 * 60;
                        if (soundOn && collectStarSound) {
                            Mix_PlayChannel(-1, collectStarSound, 0);
                        }
                    }

                    if (stars[i].pos.y > 576) {
                        stars[i].active = false;
                    }
                }
            }
            stars.erase(
                remove_if(stars.begin(), stars.end(), [](const Star& s) { return !s.active; }),
                stars.end()
            );

            // Countdown thoi gian Razzy Mode
            if (razzyModeMessageTimer > 0) {
                razzyModeMessageTimer--;
            }
        }

        // Clear man hinh
        SDL_RenderClear(renderer);
        if (gameState == MENU) {
            renderMenu(renderer, font, back0_texture, menuButtons, showInstructionsPanel, closeInstructionsButton);
        } else {
            if (back1_texture) {
                SDL_RenderCopy(renderer, back1_texture, NULL, NULL);
            }

            // Hien thi Game Over
            if (gameState == GAME_OVER && font) {
                string gameOverText = "Game Over";
                SDL_Surface* gameOverSurface = TTF_RenderText_Blended(font, gameOverText.c_str(), white);
                if (gameOverSurface) {
                    SDL_Texture* gameOverTexture = SDL_CreateTextureFromSurface(renderer, gameOverSurface);
                    if (gameOverTexture) {
                        SDL_Rect gameOverRect = {(1024 - gameOverSurface->w) / 2, (576 - gameOverSurface->h) / 2 - 60, gameOverSurface->w, gameOverSurface->h};
                        SDL_RenderCopy(renderer, gameOverTexture, NULL, &gameOverRect);
                        SDL_DestroyTexture(gameOverTexture);
                    }
                    SDL_FreeSurface(gameOverSurface);
                }

                string finalScoreText = "Your Score: " + to_string(score);
                SDL_Surface* finalScoreSurface = TTF_RenderText_Blended(font, finalScoreText.c_str(), white);
                if (finalScoreSurface) {
                    SDL_Texture* finalScoreTexture = SDL_CreateTextureFromSurface(renderer, finalScoreSurface);
                    if (finalScoreTexture) {
                        SDL_Rect finalScoreRect = {(1024 - finalScoreSurface->w) / 2, (576 - finalScoreSurface->h) / 2 - 20, finalScoreSurface->w, finalScoreSurface->h};
                        SDL_RenderCopy(renderer, finalScoreTexture, NULL, &finalScoreRect);
                        SDL_DestroyTexture(finalScoreTexture);
                    }
                    SDL_FreeSurface(finalScoreSurface);
                }

                string highscoreText = "Highscore: " + to_string(highscore);
                SDL_Surface* highscoreSurface = TTF_RenderText_Blended(font, highscoreText.c_str(), white);
                if (highscoreSurface) {
                    SDL_Texture* highscoreTexture = SDL_CreateTextureFromSurface(renderer, highscoreSurface);
                    if (highscoreTexture) {
                        SDL_Rect highscoreRect = {(1024 - highscoreSurface->w) / 2, (576 - highscoreSurface->h) / 2 + 20, highscoreSurface->w, highscoreSurface->h};
                        SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
                        SDL_DestroyTexture(highscoreTexture);
                    }
                    SDL_FreeSurface(highscoreSurface);
                }

                renderButton(renderer, font, restartButton, {100, 100, 100, 255});
            } else {
                // Hien thi qua cau lua (fireballs)
                if (fireball_texture) {
                    for (const auto& fireball : fireballs) {
                        if (fireball.active) {
                            SDL_Rect fireballRect = {fireball.pos.x, fireball.pos.y, fireballWidth, fireballHeight};
                            SDL_RenderCopy(renderer, fireball_texture, NULL, &fireballRect);
                        }
                    }
                }

                // Hien thi sao (stars)
                if (star_texture) {
                    for (const auto& star : stars) {
                        if (star.active) {
                            SDL_Rect starRect = {star.pos.x, star.pos.y, starWidth, starHeight};
                            SDL_RenderCopy(renderer, star_texture, NULL, &starRect);
                        }
                    }
                }

                // Hien thi nhan vat (player)
                SDL_Rect playerRect = {playerPos.x, playerPos.y, 64, 64};
                if (horizontalVelocity != 0 && player_run_texture) {
                    SDL_RenderCopyEx(renderer, player_run_texture, NULL, &playerRect, 0, NULL,
                                     isMovingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);
                } else if (isJumping && player_jump_texture) {
                    SDL_RenderCopy(renderer, player_jump_texture, NULL, &playerRect);
                } else if (player_stop_texture) {
                    SDL_RenderCopyEx(renderer, player_stop_texture, NULL, &playerRect, 0, NULL,
                                     isMovingRight ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL);
                }

                // Hien thi mang (trai tim)
                if (heart_texture) {
                    for (int i = 0; i < lives; i++) {
                        SDL_Rect heartRect = {10 + i * (heartWidth + 5), 10, heartWidth, heartHeight};
                        SDL_RenderCopy(renderer, heart_texture, NULL, &heartRect);
                    }
                }

                // Hien thi diem hien tai
                if (font) {
                    string scoreText = "Current Score: " + to_string(score);
                    SDL_Surface* scoreSurface = TTF_RenderText_Blended(font, scoreText.c_str(), white);
                    if (scoreSurface) {
                        SDL_Texture* scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface);
                        if (scoreTexture) {
                            SDL_Rect scoreRect = {10, 50, scoreSurface->w, scoreSurface->h};
                            SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect);
                            SDL_DestroyTexture(scoreTexture);
                        }
                        SDL_FreeSurface(scoreSurface);
                    }
                }

                // Hien thi diem cao nhat
                if (font) {
                    string highscoreText = "Highscore: " + to_string(highscore);
                    SDL_Surface* highscoreSurface = TTF_RenderText_Blended(font, highscoreText.c_str(), white);
                    if (highscoreSurface) {
                        SDL_Texture* highscoreTexture = SDL_CreateTextureFromSurface(renderer, highscoreSurface);
                        if (highscoreTexture) {
                            SDL_Rect highscoreRect = {10, 80, highscoreSurface->w, highscoreSurface->h};
                            SDL_RenderCopy(renderer, highscoreTexture, NULL, &highscoreRect);
                            SDL_DestroyTexture(highscoreTexture);
                        }
                        SDL_FreeSurface(highscoreSurface);
                    }
                }

                // Hien thi thoi gian Countdown
                if (font) {
                    int secondsLeft = countdownTime / 60;
                    string timerText = "Countdown: " + to_string(secondsLeft);
                    SDL_Surface* timerSurface = TTF_RenderText_Blended(font, timerText.c_str(), white);
                    if (timerSurface) {
                        SDL_Texture* timerTexture = SDL_CreateTextureFromSurface(renderer, timerSurface);
                        if (timerTexture) {
                            SDL_Rect timerRect = {10, 110, timerSurface->w, timerSurface->h};
                            SDL_RenderCopy(renderer, timerTexture, NULL, &timerRect);
                            SDL_DestroyTexture(timerTexture);
                        }
                        SDL_FreeSurface(timerSurface);
                    }
                }

                // Hien thi thong bao Razzy Mode
                if (razzyModeMessageTimer > 0 && font) {
                    string razzyText = "Crazy mode!";
                    SDL_Surface* razzySurface = TTF_RenderText_Blended(font, razzyText.c_str(), white);
                    if (razzySurface) {
                        SDL_Texture* razzyTexture = SDL_CreateTextureFromSurface(renderer, razzySurface);
                        if (razzyTexture) {
                            SDL_Rect razzyRect = {(1024 - razzySurface->w) / 2, 150, razzySurface->w, razzySurface->h};
                            SDL_RenderCopy(renderer, razzyTexture, NULL, &razzyRect);
                            SDL_DestroyTexture(razzyTexture);
                        }
                        SDL_FreeSurface(razzySurface);
                    }
                }

                // Hien thi man hinh Pause
                if (gameState == PAUSED) {
                    renderPaused(renderer, font, pausedButtons, score);
                }
            }
        }

        // Render man hinh
        SDL_RenderPresent(renderer);

        // Limit FPS 60
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    // Luu diem cao nhat
    saveHighscore(highscore);

    // Don dep 
    if (back0_texture) SDL_DestroyTexture(back0_texture);
    if (back1_texture) SDL_DestroyTexture(back1_texture);
    if (player_jump_texture) SDL_DestroyTexture(player_jump_texture);
    if (player_run_texture) SDL_DestroyTexture(player_run_texture);
    if (player_stop_texture) SDL_DestroyTexture(player_stop_texture);
    if (fireball_texture) SDL_DestroyTexture(fireball_texture);
    if (heart_texture) SDL_DestroyTexture(heart_texture);
    if (star_texture) SDL_DestroyTexture(star_texture);
    if (font) TTF_CloseFont(font);
    if (backgroundMusic) Mix_FreeMusic(backgroundMusic);
    if (jumpSound) Mix_FreeChunk(jumpSound);
    if (collectStarSound) Mix_FreeChunk(collectStarSound);
    if (hitSound) Mix_FreeChunk(hitSound);
    if (pauseSound) Mix_FreeChunk(pauseSound);
    Mix_CloseAudio();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
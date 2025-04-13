#include <bits/stdc++.h>
#include "src/include/SDL2/SDL.h"
#include "src/include/SDL2/SDL_image.h"
#include "src/include/SDL2/SDL_timer.h"
#include "src/include/SDL2/SDL_ttf.h"
#include "src/include/SDL2/SDL_mixer.h"
#include <fstream>

#undef main
using namespace std;

// Cấu trúc lưu vị trí (x, y)
struct Position {
    int x, y;
};

// Cấu trúc cho quả cầu lửa
struct Fireball {
    Position pos;        // Vị trí của quả cầu lửa
    float velocityX, velocityY; // Vận tốc theo trục x và y
    bool active;         // Trạng thái hoạt động
    bool scored;         // Đã được tính điểm hay chưa
};

// Cấu trúc cho ngôi sao
struct Star {
    Position pos;        // Vị trí của ngôi sao
    float velocityY;     // Vận tốc rơi theo trục y
    bool active;         // Trạng thái hoạt động
};

// Hàm đọc điểm cao nhất từ file
int loadHighscore() {
    ifstream file("highscore.txt");
    int highscore = 0;
    if (file.is_open()) {
        file >> highscore;
        file.close();
    }
    return highscore;
}

// Hàm lưu điểm cao nhất vào file
void saveHighscore(int highscore) {
    ofstream file("highscore.txt");
    if (file.is_open()) {
        file << highscore;
        file.close();
    } else {
        cerr << "Không thể mở file để lưu điểm cao nhất!" << endl;
    }
}

int main(int argc, char* argv[]) {
    // Khởi tạo SDL với video và âm thanh
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        cerr << "SDL không thể khởi tạo! SDL_Error: " << SDL_GetError() << endl;
        return 1;
    }

    // Khởi tạo SDL_image
    if (!(IMG_Init(IMG_INIT_PNG))) {
        cerr << "SDL_image không thể khởi tạo! IMG_Error: " << IMG_GetError() << endl;
        return 1;
    }

    // Khởi tạo SDL_ttf
    if (TTF_Init() < 0) {
        cerr << "SDL_ttf không thể khởi tạo! TTF_Error: " << TTF_GetError() << endl;
        return 1;
    }

    // Khởi tạo SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        cerr << "SDL_mixer không thể khởi tạo! SDL_Error: " << Mix_GetError() << endl;
        return 1;
    }

    // Tạo cửa sổ game
    SDL_Window* window = SDL_CreateWindow("DINO RUN",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          1024, 576, SDL_WINDOW_SHOWN);
    if (!window) {
        cerr << "Không thể tạo cửa sổ! SDL_Error: " << SDL_GetError() << endl;
        return 1;
    }

    // Tạo renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        cerr << "Renderer không thể tạo ra! SDL_Error: " << SDL_GetError() << endl;
        return 1;
    }

    // Tải font chữ
    TTF_Font* font = TTF_OpenFont("font.ttf", 20);
    if (!font) {
        cerr << "Không thể tải font! TTF_Error: " << TTF_GetError() << endl;
    }

    // Tải các texture
    SDL_Texture* back0_texture = IMG_LoadTexture(renderer, "images/back0.png");
    if (!back0_texture) {
        cerr << "Không thể tải back0.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* back1_texture = IMG_LoadTexture(renderer, "images/back1.png");
    if (!back1_texture) {
        cerr << "Không thể tải back1.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* player_jump_texture = IMG_LoadTexture(renderer, "images/jump.png");
    if (!player_jump_texture) {
        cerr << "Không thể tải jump.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* player_run_texture = IMG_LoadTexture(renderer, "images/run.png");
    if (!player_run_texture) {
        cerr << "Không thể tải run.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* player_stop_texture = IMG_LoadTexture(renderer, "images/stop.png");
    if (!player_stop_texture) {
        cerr << "Không thể tải stop.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* fireball_texture = IMG_LoadTexture(renderer, "images/fireball.png");
    if (!fireball_texture) {
        cerr << "Không thể tải fireball.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* heart_texture = IMG_LoadTexture(renderer, "images/tim.png");
    if (!heart_texture) {
        cerr << "Không thể tải tim.png! SDL_Error: " << SDL_GetError() << endl;
    }

    SDL_Texture* star_texture = IMG_LoadTexture(renderer, "images/sao.png");
    if (!star_texture) {
        cerr << "Không thể tải sao.png! SDL_Error: " << SDL_GetError() << endl;
    }

    // Tải âm thanh
    Mix_Music* backgroundMusic = Mix_LoadMUS("C:\\ProjectsSDL\\DemoSDL\\audio\\background_music.mp3");
    if (!backgroundMusic) {
        cerr << "Không thể tải background music! SDL_Error: " << Mix_GetError() << endl;
    } else {
        Mix_PlayMusic(backgroundMusic, -1); // Phát nhạc nền lặp lại
    }

    Mix_Chunk* jumpSound = Mix_LoadWAV("C:\\ProjectsSDL\\DemoSDL\\audio\\jump.wav");
    if (!jumpSound) {
        cerr << "Không thể tải jump.wav! SDL_Error: " << Mix_GetError() << endl;
    }

    Mix_Chunk* collectStarSound = Mix_LoadWAV("C:\\ProjectsSDL\\DemoSDL\\audio\\collect_star.wav");
    if (!collectStarSound) {
        cerr << "Không thể tải collect_star.wav! SDL_Error: " << Mix_GetError() << endl;
    }

    Mix_Chunk* hitSound = Mix_LoadWAV("C:\\ProjectsSDL\\DemoSDL\\audio\\hit.wav");
    if (!hitSound) {
        cerr << "Không thể tải hit.wav! SDL_Error: " << Mix_GetError() << endl;
    }

    Mix_Chunk* pauseSound = Mix_LoadWAV("C:\\ProjectsSDL\\DemoSDL\\audio\\pause.wav");
    if (!pauseSound) {
        cerr << "Không thể tải pause.wav! SDL_Error: " << Mix_GetError() << endl;
    }

    // Các biến game
    bool quit = false;           // Biến để thoát game
    bool paused = false;         // Trạng thái tạm dừng
    bool gameOver = false;       // Trạng thái game over
    int razzyModeMessageTimer = 0; // Thời gian hiển thị thông báo "Razzy Mode!"
    const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL); // Trạng thái phím
    const int frameDelay = 1000 / 60; // Giới hạn FPS (60 FPS)

    // Vị trí người chơi
    Position playerPos = {400, 400}; // Vị trí ban đầu
    bool isJumping = false;          // Trạng thái nhảy
    bool isMovingRight = true;       // Hướng di chuyển
    float horizontalVelocity = 0.0f; // Vận tốc ngang
    float verticalVelocity = 0.0f;   // Vận tốc dọc
    const float moveSpeed = 5.0f;    // Tốc độ di chuyển ngang
    const float jumpForce = -13.0f;  // Lực nhảy
    const float gravity = 0.5f;      // Trọng lực

    // Kích thước trái tim (mạng)
    const int heartWidth = 32;
    const int heartHeight = 32;

    // Biến cho quả cầu lửa
    vector<Fireball> fireballs;      // Danh sách quả cầu lửa
    const int fireballWidth = 50;    // Chiều rộng quả cầu lửa
    const int fireballHeight = 50;   // Chiều cao quả cầu lửa
    float fireballSpeed = 3.0f;      // Tốc độ quả cầu lửa
    int spawnTimer = 0;              // Thời gian để sinh quả cầu lửa
    int spawnInterval = 120;         // Khoảng thời gian giữa các lần sinh

    // Biến cho ngôi sao
    vector<Star> stars;              // Danh sách ngôi sao
    const int starWidth = 64;        // Chiều rộng ngôi sao
    const int starHeight = 64;       // Chiều cao ngôi sao
    const float starFallSpeed = 2.0f;// Tốc độ rơi của ngôi sao
    const int starSpawnInterval = 300; // Khoảng thời gian giữa các lần sinh ngôi sao
    int starSpawnTimer = 0;          // Thời gian để sinh ngôi sao

    // Thời gian game
    int countdownTime = 20 * 60;     // Thời gian đếm ngược (30 giây)
    bool razzyMode = false;          // Chế độ Razzy

    // Số mạng
    int lives = 5;

    // Điểm số
    int score = 0;
    int highscore = loadHighscore();

    // Màu sắc
    SDL_Color white = {255, 255, 255, 255};

    // Hiển thị màn hình bắt đầu
    if (font) {
        string startText = "Press to Space to Start";
        SDL_Surface* startSurface = TTF_RenderText_Blended(font, startText.c_str(), white);
        if (!startSurface) {
            cerr << "Không thể tạo surface cho text! TTF_Error: " << TTF_GetError() << endl;
        } else {
            SDL_Texture* startTexture = SDL_CreateTextureFromSurface(renderer, startSurface);
            if (!startTexture) {
                cerr << "Không thể tạo texture cho text! SDL_Error: " << SDL_GetError() << endl;
            } else {
                SDL_Rect startRect = {(1024 - startSurface->w) / 2, 576 - startSurface->h - 135, startSurface->w, startSurface->h};
                if (back0_texture) {
                    SDL_RenderCopy(renderer, back0_texture, NULL, NULL);
                }
                SDL_RenderCopy(renderer, startTexture, NULL, &startRect);

                // Thêm hướng dẫn chơi (đã xóa "S: Rơi Nhanh")
                string instructionsText = "A/D: Move, W: Jump, P: Pause";
                SDL_Surface* instructionsSurface = TTF_RenderText_Blended(font, instructionsText.c_str(), white);
                if (instructionsSurface) {
                    SDL_Texture* instructionsTexture = SDL_CreateTextureFromSurface(renderer, instructionsSurface);
                    if (instructionsTexture) {
                        SDL_Rect instructionsRect = {(1024 - instructionsSurface->w) / 2, 576 - instructionsSurface->h - 100, instructionsSurface->w, instructionsSurface->h};
                        SDL_RenderCopy(renderer, instructionsTexture, NULL, &instructionsRect);
                        SDL_DestroyTexture(instructionsTexture);
                    }
                    SDL_FreeSurface(instructionsSurface);
                }

                SDL_RenderPresent(renderer);
                SDL_DestroyTexture(startTexture);
            }
            SDL_FreeSurface(startSurface);
        }
    } else {
        if (back0_texture) {
            SDL_RenderCopy(renderer, back0_texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
    }

    SDL_Event startEvent;
    bool gameStarted = false;

    // Chờ người chơi nhấn SPACE để bắt đầu
    while (!gameStarted) {
        while (SDL_PollEvent(&startEvent)) {
            if (startEvent.type == SDL_KEYDOWN && startEvent.key.keysym.sym == SDLK_SPACE) {
                gameStarted = true;
            }
            if (startEvent.type == SDL_QUIT) {
                // Dọn dẹp tài nguyên trước khi thoát
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
        }
    }

    // Vòng lặp chính của game
    SDL_Event e;
    while (!quit) {
        Uint32 frameStart = SDL_GetTicks();

        // Xử lý sự kiện
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_w && !isJumping && !paused && !gameOver) {
                    isJumping = true;
                    verticalVelocity = jumpForce;
                    if (jumpSound) Mix_PlayChannel(-1, jumpSound, 0);
                }
                if (e.key.keysym.sym == SDLK_p && !gameOver) {
                    paused = !paused;
                    if (pauseSound) Mix_PlayChannel(-1, pauseSound, 0);
                }
                if (e.key.keysym.sym == SDLK_r && gameOver) {
                    // Đặt lại trạng thái game
                    gameOver = false;
                    paused = false;
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

        // Bỏ qua cập nhật game nếu đang tạm dừng hoặc game over
        if (!paused && !gameOver) {
            // Di chuyển ngang
            horizontalVelocity = 0.0f;
            if (currentKeyStates[SDL_SCANCODE_A]) {
                horizontalVelocity = -moveSpeed;
                isMovingRight = false;
            }
            if (currentKeyStates[SDL_SCANCODE_D]) {
                horizontalVelocity = moveSpeed;
                isMovingRight = true;
            }

            // Cập nhật vị trí ngang
            playerPos.x += static_cast<int>(horizontalVelocity);
            if (playerPos.x < 0) playerPos.x = 0;
            if (playerPos.x > 1024 - 64) playerPos.x = 1024 - 64;

            // Cập nhật vị trí dọc (nhảy)
            if (isJumping) {
                playerPos.y += static_cast<int>(verticalVelocity);
                verticalVelocity += gravity;
            }
            if (playerPos.y >= 400) {
                playerPos.y = 400;
                isJumping = false;
                verticalVelocity = 0.0f;
            }

            // Logic đếm ngược và chế độ Razzy
            if (!razzyMode) {
                countdownTime--;
                if (countdownTime <= 0) {
                    razzyMode = true;
                    fireballSpeed = 5.0f;
                    spawnInterval = 60;
                    razzyModeMessageTimer = 180; // Hiển thị "Razzy Mode!" trong 3 giây
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

            // Sinh quả cầu lửa
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

            // Cập nhật quả cầu lửa
            for (int i = 0; i < fireballs.size(); i++) {
                if (fireballs[i].active) {
                    fireballs[i].pos.x += static_cast<int>(fireballs[i].velocityX);
                    fireballs[i].pos.y += static_cast<int>(fireballs[i].velocityY);

                    SDL_Rect fireballRect = {fireballs[i].pos.x, fireballs[i].pos.y, fireballWidth, fireballHeight};
                    SDL_Rect playerRect = {playerPos.x, playerPos.y, 64, 64};
                    if (SDL_HasIntersection(&fireballRect, &playerRect)) {
                        fireballs[i].active = false;
                        lives--;
                        if (hitSound) Mix_PlayChannel(-1, hitSound, 0);
                        if (lives <= 0) {
                            gameOver = true;
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

            // Sinh ngôi sao
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

            // Cập nhật ngôi sao
            for (int i = 0; i < stars.size(); i++) {
                if (stars[i].active) {
                    stars[i].pos.y += static_cast<int>(stars[i].velocityY);

                    SDL_Rect starRect = {stars[i].pos.x, stars[i].pos.y, starWidth, starHeight};
                    SDL_Rect playerRect = {playerPos.x, playerPos.y, 64, 64};
                    if (SDL_HasIntersection(&starRect, &playerRect)) {
                        stars[i].active = false;
                        countdownTime += 5 * 60;
                        if (countdownTime > 20 * 60) countdownTime = 20 * 60;
                        if (collectStarSound) Mix_PlayChannel(-1, collectStarSound, 0);
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

            // Cập nhật thời gian hiển thị thông báo Razzy Mode
            if (razzyModeMessageTimer > 0) {
                razzyModeMessageTimer--;
            }
        }

        // Xóa màn hình
        SDL_RenderClear(renderer);
        if (back1_texture) {
            SDL_RenderCopy(renderer, back1_texture, NULL, NULL);
        }

        // Hiển thị màn hình game over
        if (gameOver && font) {
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

            string restartText = "Press R to restart";
            SDL_Surface* restartSurface = TTF_RenderText_Blended(font, restartText.c_str(), white);
            if (restartSurface) {
                SDL_Texture* restartTexture = SDL_CreateTextureFromSurface(renderer, restartSurface);
                if (restartTexture) {
                    SDL_Rect restartRect = {(1024 - restartSurface->w) / 2, (576 - restartSurface->h) / 2 + 60, restartSurface->w, restartSurface->h};
                    SDL_RenderCopy(renderer, restartTexture, NULL, &restartRect);
                    SDL_DestroyTexture(restartTexture);
                }
                SDL_FreeSurface(restartSurface);
            }
        } else {
            // Hiển thị quả cầu lửa
            if (fireball_texture) {
                for (const auto& fireball : fireballs) {
                    if (fireball.active) {
                        SDL_Rect fireballRect = {fireball.pos.x, fireball.pos.y, fireballWidth, fireballHeight};
                        SDL_RenderCopy(renderer, fireball_texture, NULL, &fireballRect);
                    }
                }
            }

            // Hiển thị ngôi sao
            if (star_texture) {
                for (const auto& star : stars) {
                    if (star.active) {
                        SDL_Rect starRect = {star.pos.x, star.pos.y, starWidth, starHeight};
                        SDL_RenderCopy(renderer, star_texture, NULL, &starRect);
                    }
                }
            }

            // Hiển thị người chơi
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

            // Hiển thị số mạng (trái tim)
            if (heart_texture) {
                for (int i = 0; i < lives; i++) {
                    SDL_Rect heartRect = {10 + i * (heartWidth + 5), 10, heartWidth, heartHeight};
                    SDL_RenderCopy(renderer, heart_texture, NULL, &heartRect);
                }
            }

            // Hiển thị điểm số
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

            // Hiển thị điểm cao nhất
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

            // Hiển thị thời gian đếm ngược
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

            // Hiển thị thông báo chế độ Razzy
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

            // Hiển thị màn hình tạm dừng
            if (paused && font) {
                string pauseText = "Paused";
                SDL_Surface* pauseSurface = TTF_RenderText_Blended(font, pauseText.c_str(), white);
                if (pauseSurface) {
                    SDL_Texture* pauseTexture = SDL_CreateTextureFromSurface(renderer, pauseSurface);
                    if (pauseTexture) {
                        SDL_Rect pauseRect = {(1024 - pauseSurface->w) / 2, (576 - pauseSurface->h) / 2 - 20, pauseSurface->w, pauseSurface->h};
                        SDL_RenderCopy(renderer, pauseTexture, NULL, &pauseRect);
                        SDL_DestroyTexture(pauseTexture);
                    }
                    SDL_FreeSurface(pauseSurface);
                }

                string pauseScoreText = "Current Score: " + to_string(score);
                SDL_Surface* pauseScoreSurface = TTF_RenderText_Blended(font, pauseScoreText.c_str(), white);
                if (pauseScoreSurface) {
                    SDL_Texture* pauseScoreTexture = SDL_CreateTextureFromSurface(renderer, pauseScoreSurface);
                    if (pauseScoreTexture) {
                        SDL_Rect pauseScoreRect = {(1024 - pauseScoreSurface->w) / 2, (576 - pauseScoreSurface->h) / 2 + 20, pauseScoreSurface->w, pauseScoreSurface->h};
                        SDL_RenderCopy(renderer, pauseScoreTexture, NULL, &pauseScoreRect);
                        SDL_DestroyTexture(pauseScoreTexture);
                    }
                    SDL_FreeSurface(pauseScoreSurface);
                }
            }
        }

        // Hiển thị renderer
        SDL_RenderPresent(renderer);

        // Giới hạn FPS ở mức 60
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    // Lưu điểm cao nhất trước khi thoát
    saveHighscore(highscore);

    // Dọn dẹp tài nguyên
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
#include <SDL2/SDL_main.h>
#include <SDL2/SDL.h>
#include <SDL2_image/SDL_image.h>
#include <SDL2_ttf/SDL_ttf.h>
#include <SDL2_mixer/SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 640 x 480 square pixel
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define TILE_SIZE 20

// x, y coordinate
typedef struct {
    int x, y;
} Point;

typedef struct {
    Point body[SCREEN_WIDTH * SCREEN_HEIGHT / (TILE_SIZE * TILE_SIZE)];
    int length;
    Point direction;
} Snake;


SDL_Texture *load_texture(SDL_Renderer *renderer, const char *file) {
    SDL_Surface *surface = IMG_Load(file);
    if (!surface) {
        printf("IMG_Load Error: %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}


void render_text(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color, int x, int y) {
    SDL_Surface *surface = TTF_RenderText_Solid(font, text, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dest = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

// Function to render the Game Over screen
void render_game_over(SDL_Renderer *renderer, TTF_Font *font, int score) {
    // Clearing the screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black background
    SDL_RenderClear(renderer);

    // Rendering "Game Over" text
    render_text(renderer, font, "Game Over!", (SDL_Color){255, 0, 0, 255}, 240, 190);

    // Rendering the final score
    char scoreText[64];
    sprintf(scoreText, "Final Score: %d", score);
    render_text(renderer, font, scoreText, (SDL_Color){255, 255, 255, 255}, 240, 240);

    // Displaying the instructions to exit
    render_text(renderer, font, "Press ESC to exit...", (SDL_Color){200, 200, 200, 255}, 240, 290);

    SDL_RenderPresent(renderer);

    // Waiting for the user to press ESC
    SDL_Event e;
    bool waiting = true;
    while (waiting) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
                waiting = false;
                break;
            }
        }
        SDL_Delay(100);
    }
}

int main() {
    //Initialization Checking
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 || TTF_Init() == -1 || Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
        return 1;

    SDL_Window *window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (!window || !renderer) return 1;

    TTF_Font *font = TTF_OpenFont("arial.ttf", 24);
    Mix_Chunk *eatSound = Mix_LoadWAV("eat.wav");

    SDL_Texture *headTextures[4];
    SDL_Texture *tailTextures[4];
    SDL_Texture *bodyTextures[6];

    headTextures[0] = load_texture(renderer, "texture/head_up.png");
    headTextures[1] = load_texture(renderer, "texture/head_down.png");
    headTextures[2] = load_texture(renderer, "texture/head_left.png");
    headTextures[3] = load_texture(renderer, "texture/head_right.png");

    tailTextures[0] = load_texture(renderer, "texture/tail_down.png");
    tailTextures[1] = load_texture(renderer, "texture/tail_up.png");
    tailTextures[2] = load_texture(renderer, "texture/tail_right.png");
    tailTextures[3] = load_texture(renderer, "texture/tail_left.png");

    bodyTextures[0] = load_texture(renderer, "texture/body_vertical.png");
    bodyTextures[1] = load_texture(renderer, "texture/body_horizontal.png");
    bodyTextures[2] = load_texture(renderer, "texture/body_topleft.png");
    bodyTextures[3] = load_texture(renderer, "texture/body_topright.png");
    bodyTextures[4] = load_texture(renderer, "texture/body_bottomleft.png");
    bodyTextures[5] = load_texture(renderer, "texture/body_bottomright.png");

    SDL_Texture *foodTexture = load_texture(renderer, "food.png");
    if (!font || !eatSound || !foodTexture) return 1;

    srand(time(NULL)); //random number generator
    
    Snake snake = {{0}, 5, {1, 0}};
    for (int i = 0; i < snake.length; ++i)
        snake.body[i] = (Point){snake.length - i - 1, 0};

    Point food = {rand() % (SCREEN_WIDTH / TILE_SIZE), rand() % (SCREEN_HEIGHT / TILE_SIZE)};

    SDL_Event e;
    int score = 0;
    bool running = true;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_UP: if (snake.direction.y == 0) snake.direction = (Point){0, -1}; break;  //Going upward
                    case SDLK_DOWN: if (snake.direction.y == 0) snake.direction = (Point){0, 1}; break; //Going downwar
                    case SDLK_LEFT: if (snake.direction.x == 0) snake.direction = (Point){-1, 0}; break; //Going left
                    case SDLK_RIGHT: if (snake.direction.x == 0) snake.direction = (Point){1, 0}; break; //Going right
                }
            }
        }

        //Moving the Body
        for (int i = snake.length - 1; i > 0; --i)
            snake.body[i] = snake.body[i - 1];
        //Moving the Head
        snake.body[0].x += snake.direction.x;
        snake.body[0].y += snake.direction.y;

        //Checking boundary collision
        if (snake.body[0].x < 0 || snake.body[0].x >= SCREEN_WIDTH / TILE_SIZE ||
            snake.body[0].y < 0 || snake.body[0].y >= SCREEN_HEIGHT / TILE_SIZE)
            running = false;

        //Checking self collision    
        for (int i = 1; i < snake.length; ++i)
            if (snake.body[0].x == snake.body[i].x && snake.body[0].y == snake.body[i].y)
                running = false;
        
        if (snake.body[0].x == food.x && snake.body[0].y == food.y) {
            snake.length++;
            score++;
            food = (Point){rand() % (SCREEN_WIDTH / TILE_SIZE), rand() % (SCREEN_HEIGHT / TILE_SIZE)};
            Mix_PlayChannel(-1, eatSound, 0);
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect foodRect = {food.x * TILE_SIZE, food.y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
        SDL_RenderCopy(renderer, foodTexture, NULL, &foodRect);

        //Loop Through Each Segment
        for (int i = 0; i < snake.length; ++i) {
            SDL_Rect snakeRect = {snake.body[i].x * TILE_SIZE, snake.body[i].y * TILE_SIZE, TILE_SIZE, TILE_SIZE};
            SDL_Texture *segmentTexture = NULL;

            if (i == 0) {
                if (snake.direction.x == 1) segmentTexture = headTextures[3]; //right
                else if (snake.direction.x == -1) segmentTexture = headTextures[2]; //left
                else if (snake.direction.y == 1) segmentTexture = headTextures[1]; //downward
                else if (snake.direction.y == -1) segmentTexture = headTextures[0]; //upward
            }
            
            else if (i == snake.length - 1) {
                Point prev = snake.body[i - 1];

                if (snake.body[i].x < prev.x) segmentTexture = tailTextures[3]; //moving right
                else if (snake.body[i].x > prev.x) segmentTexture = tailTextures[2]; //moving left
                else if (snake.body[i].y < prev.y) segmentTexture = tailTextures[1]; //moving downward
                else if (snake.body[i].y > prev.y) segmentTexture = tailTextures[0]; //moving upward
            }
            
            else {
                Point prev = snake.body[i - 1];
                Point next = snake.body[i + 1];

                if (prev.x == next.x) segmentTexture = bodyTextures[0]; //vertical
                else if (prev.y == next.y) segmentTexture = bodyTextures[1]; //Horizontal
                
                //checking corner
                
                //turns right after moving up, or, turns downward after moving left
                else if ((prev.x < snake.body[i].x && next.y < snake.body[i].y) || 
                         (next.x < snake.body[i].x && prev.y < snake.body[i].y))
                    segmentTexture = bodyTextures[2];
                
                //turns right after moving down, or, turns upward after moving left
                else if ((prev.x < snake.body[i].x && next.y > snake.body[i].y) || 
                         (next.x < snake.body[i].x && prev.y > snake.body[i].y))
                    segmentTexture = bodyTextures[4];
                    
                //turns left after moving down, or, turns upward after moving right    
                else if ((prev.x > snake.body[i].x && next.y > snake.body[i].y) || 
                         (next.x > snake.body[i].x && prev.y > snake.body[i].y))
                    segmentTexture = bodyTextures[5];

                //left after moving up ,or, downward after moving right
                else if ((prev.x > snake.body[i].x && next.y < snake.body[i].y) || 
                         (next.x > snake.body[i].x && prev.y < snake.body[i].y))
                    segmentTexture = bodyTextures[3];
            }
            SDL_RenderCopy(renderer, segmentTexture, NULL, &snakeRect);
        }

        char scoreText[32];
        sprintf(scoreText, "Score: %d", score);
        render_text(renderer, font, scoreText, (SDL_Color){255, 255, 255, 255}, 10, 10);

        SDL_RenderPresent(renderer);
        SDL_Delay(100);
    }

    // Render the Game Over screen
    render_game_over(renderer, font, score);

    SDL_DestroyTexture(foodTexture);
    for (int i = 0; i < 4; ++i) {
        SDL_DestroyTexture(headTextures[i]);
        SDL_DestroyTexture(tailTextures[i]);
    }
    for (int i = 0; i < 6; ++i)
        SDL_DestroyTexture(bodyTextures[i]);

    Mix_FreeChunk(eatSound);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    Mix_CloseAudio();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}

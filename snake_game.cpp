#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define FPS 20
#define SCREEN_WIDTH 680
#define SCREEN_HEIGHT 400
#define WALL_THICKNESS 20
#define CELL_WIDTH 20
#define CELL_HEIGHT 20
#define CELL_COUNT                                                             \
  ((SCREEN_WIDTH - WALL_THICKNESS * 2) *                                       \
   (SCREEN_HEIGHT - WALL_THICKNESS * 2)) /                                     \
      (CELL_WIDTH * CELL_HEIGHT)
#define SNAKE_START_X (SCREEN_WIDTH - WALL_THICKNESS * 2) / 2
#define SNAKE_START_Y (SCREEN_HEIGHT - WALL_THICKNESS * 2) / 2

void initialize();
void terminate(int exit_code);
void handle_input();
void draw_walls();
void spawn_snake();
void draw_snake();
void move_snake();
void change_direction(SDL_Keycode new_direction);
void handle_collisions();
void play_again();
void spawn_food();
void draw_food();
void get_text_and_rect(SDL_Renderer *renderer, int x, int y, const char *text,
                       TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect, SDL_Color textColor);
void print_score();
void print_help();

typedef enum {
  NOT_PLAYING,
  PLAYING,
  PAUSED,
  GAME_OVER,
} Game_state;

typedef struct Game
{
  SDL_Window *window;
  SDL_Renderer *renderer;
  bool running;
  SDL_Rect snake[CELL_COUNT];
  Game_state state;
  int dx;
  int dy;
  SDL_Rect food;
  SDL_Texture *apple;
  int score;
  int highest_score;
  TTF_Font *font;
  SDL_Color textColor;
} Game;

Game game = { .running = true,
              .state = NOT_PLAYING,
              .dx = CELL_WIDTH,
              .food = { .w = CELL_WIDTH, .h = CELL_HEIGHT },
              .score = 0,
              .highest_score = 0};

int main(int argc, char *argv[])
{
  initialize();

  spawn_snake();
  spawn_food();

  while (game.running)
  {
    // clean screen to black before drawing eveything
    SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
    SDL_RenderClear(game.renderer);

    handle_input();
    move_snake();

    draw_food();
    draw_snake();
    draw_walls();
    print_help();
    print_score();

    SDL_RenderPresent(game.renderer);
    SDL_Delay(1000/FPS);
  }

  terminate(EXIT_SUCCESS);
}

void initialize()
{
  if (SDL_Init(SDL_INIT_EVERYTHING < 0))
  {
    printf("Error: Failed to initialize SDL: %s\n", SDL_GetError());
    terminate(EXIT_FAILURE);
  }

  if (IMG_Init(IMG_INIT_PNG) < 0)
  {
    printf("Error: Failed to initialize SDL_image: %s\n", SDL_GetError());
    terminate(EXIT_FAILURE);
  }

  if (TTF_Init() < 0)
  {
    printf("Error: Failed to initialize SDL_ttf: %s\n", SDL_GetError());
    terminate(EXIT_FAILURE);
  }

  game.window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH,
                                 SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (!game.window)
  {
    printf("Erorr: Failed to create window: %s\n", SDL_GetError());
    terminate(EXIT_FAILURE);
  }

  SDL_Surface *window_icon = IMG_Load("res/snake.png");
  if (!window_icon)
  {
    printf("Erorr: Failed to load icon image: %s\n", SDL_GetError());
    terminate(EXIT_FAILURE);
  }
  SDL_SetWindowIcon(game.window, window_icon);

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  game.renderer = SDL_CreateRenderer(game.window, -1, SDL_RENDERER_ACCELERATED);
  if (!game.renderer)
  {
    printf("Error: Failed to create renderer: %s\n", SDL_GetError());
    terminate(EXIT_FAILURE);
  }

  // load font
  game.font = TTF_OpenFont("fonts/JetBrainsMono-Regular.ttf", 16);
  if (!game.font)
  {
    printf("Error: Failed to load font: %s\n", TTF_GetError());
    terminate(EXIT_FAILURE);
  }
}

void get_text_and_rect(SDL_Renderer *renderer, int x, int y, const char *text,
                       TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect, SDL_Color textColor)
{
  int text_width;
  int text_height;
  SDL_Surface *surface;

  surface = TTF_RenderText_Blended(font, text, textColor);
  *texture = SDL_CreateTextureFromSurface(renderer, surface);
  text_width = surface->w;
  text_height = surface->h;
  SDL_FreeSurface(surface);
  rect->x = x;
  rect->y = y;
  rect->w = text_width;
  rect->h = text_height;
}

void print_score()
{
  SDL_Rect rect1, rect2;
  SDL_Texture *texture1, *texture2;

  // print text in white
  game.textColor = { 0, 0, 0, 0 };

  // print game score
  std::string txt_score = "score: " + std::to_string(game.score);
  const char *chr_score = txt_score.c_str();
  int score_width = SCREEN_WIDTH / 5 * 4;
  int score_height = 0;

  get_text_and_rect(game.renderer, score_width, score_height, chr_score,
                    game.font, &texture1, &rect1, game.textColor);

  if (game.state != NOT_PLAYING)
  {
    SDL_RenderCopy(game.renderer, texture1, NULL, &rect1);
  }

  // print game highest score
  int tmp = game.score;
  if (game.highest_score < tmp)
    game.highest_score = tmp;

  if (game.state == GAME_OVER)
  {
    std::string txt_highest_score =
        "highest score: " + std::to_string(game.highest_score);
    const char *chr_highest_score = txt_highest_score.c_str();

    get_text_and_rect(game.renderer, score_width - 180, score_height,
                      chr_highest_score, game.font, &texture2, &rect2,
                      game.textColor);
    SDL_RenderCopy(game.renderer, texture2, NULL, &rect2);
  }
}

void print_help()
{
  SDL_Rect rect1, rect2, rect3;
  SDL_Texture *texture1, *texture2, *texture3;

  int help_width = WALL_THICKNESS + 20;

  // print in white
  game.textColor = { 255, 255, 255, 255 };
  // arrow key
  get_text_and_rect(game.renderer, help_width, WALL_THICKNESS,
                    "Press arrow key to play", game.font, &texture1, &rect1,
                    game.textColor);

  if (game.state == NOT_PLAYING)
  {
    // print help
    SDL_RenderCopy(game.renderer, texture1, NULL, &rect1);

    // press space to pause/resume
    get_text_and_rect(game.renderer, help_width, rect1.y + rect1.h,
                      "Press space to pause/resume", game.font, &texture2,
                      &rect2, game.textColor);
    SDL_RenderCopy(game.renderer, texture2, NULL, &rect2);
  }

  if (game.state == PAUSED)
  {
    // print in black
    game.textColor = { 0, 0, 0, 255 };
    // press space to pause/resume
    get_text_and_rect(game.renderer, help_width, 0,
                      "Press space to pause/resume", game.font, &texture2, &rect2,
                      game.textColor);
    SDL_RenderCopy(game.renderer, texture2, NULL, &rect2);
  }

  if (game.state == GAME_OVER)
  {
    // print in black
    game.textColor = { 0, 0, 0, 255 };
    get_text_and_rect(game.renderer, help_width, 0,
                      "Press space to play again", game.font, &texture3, &rect3,
                      game.textColor);
    SDL_RenderCopy(game.renderer, texture3, NULL, &rect3);

  }
}

void terminate(int exit_code)
{
  TTF_CloseFont(game.font);
  SDL_DestroyTexture(game.apple);
  SDL_DestroyRenderer(game.renderer);
  SDL_DestroyWindow(game.window);

  TTF_Quit();
  IMG_Quit();
  SDL_Quit();

  exit(exit_code);
}

void handle_input()
{
  SDL_Event e;
  while (SDL_PollEvent(&e))
  {
    if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
      game.running = false;
    if (e.type == SDL_KEYDOWN &&
        (e.key.keysym.sym == SDLK_UP || e.key.keysym.sym == SDLK_DOWN ||
         e.key.keysym.sym == SDLK_LEFT || e.key.keysym.sym == SDLK_RIGHT))
    {
      if (game.state == NOT_PLAYING)
        game.state = PLAYING;
      change_direction(e.key.keysym.sym);
    }

    if (e.type == SDL_KEYDOWN && (e.key.keysym.sym == SDLK_SPACE))
    {
      switch (game.state)
      {
        case PAUSED:
          game.state = PLAYING;
          break;
        case PLAYING:
          game.state = PAUSED;
          break;
        case GAME_OVER:
          play_again();
        default:
          break;
      }
    }
  }
}

void draw_walls()
{
  SDL_SetRenderDrawColor(game.renderer, 200, 200, 200, 255);

  SDL_Rect block = { .x = 0, .y = 0, .w = WALL_THICKNESS, .h = SCREEN_HEIGHT };

  // left wall
  SDL_RenderFillRect(game.renderer, &block);

  // right wall
  block.x = SCREEN_WIDTH - WALL_THICKNESS;
  SDL_RenderFillRect(game.renderer, &block);

  // top wall
  block.x = 0;
  block.w = SCREEN_WIDTH;
  block.h = WALL_THICKNESS;
  SDL_RenderFillRect(game.renderer, &block);

  // bottom wall
  block.y = SCREEN_HEIGHT - WALL_THICKNESS;
  SDL_RenderFillRect(game.renderer, &block);
}

void draw_snake()
{
  // draw body parts
  for (int i = 1; i < CELL_COUNT; i++)
  {
    if (game.snake[i].w != 0)
    {
      if (game.state == GAME_OVER)
      {
        SDL_SetRenderDrawColor(game.renderer, 255, 0, 0, 255);
      }
      else
      {
        SDL_SetRenderDrawColor(game.renderer, 0, 155, 0, 255);
      }
      SDL_RenderFillRect(game.renderer, &game.snake[i]);
    }

    // black border around each snake body cell
    SDL_SetRenderDrawColor(game.renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(game.renderer, &game.snake[i]);
  }

  // draw snake head
  if (game.state == GAME_OVER)
  {
    SDL_SetRenderDrawColor(game.renderer, 255, 0, 0, 255);
  }
  else
  {
    SDL_SetRenderDrawColor(game.renderer, 0, 155, 0, 255);
  }
  SDL_RenderFillRect(game.renderer, &game.snake[0]);
}

void spawn_snake()
{
  // make all body part empty
  for (int i = 0; i < CELL_COUNT; i++)
  {
    game.snake[i].x = 0;
    game.snake[i].y = 0;
    game.snake[i].w = 0;
    game.snake[i].h = 0;
  }

  // head part
  game.snake[0].x = SNAKE_START_X;
  game.snake[0].y = SNAKE_START_Y;
  game.snake[0].w = CELL_WIDTH;
  game.snake[0].h = CELL_HEIGHT;

  // create 4 more cell parts
  for (int i = 1; i < 5; i++)
  {
    game.snake[i] = game.snake[0];
    game.snake[i].x = game.snake[0].x - (CELL_WIDTH * i);
  }
}

void move_snake()
{
  if (game.state != PLAYING)
    return;

  // shift all snake body part right
  for (int i = CELL_COUNT-1; i >= 0; i--)
  {
    game.snake[i] = game.snake[i-1];
  }

  // insert new head position at the beginning
  game.snake[0].x = game.snake[1].x + game.dx;
  game.snake[0].y = game.snake[1].y + game.dy;
  game.snake[0].w = CELL_WIDTH;
  game.snake[0].h = CELL_HEIGHT;

  // find the last inactive element of the snake array
  // and zeroing out one before it
  if (game.snake[0].x == game.food.x && game.snake[0].y == game.food.y)
  {
    spawn_food();
    game.score ++;
  }
  else
  {
    for (int i = 5; i < CELL_COUNT; i++)
    {
      if (game.snake[i].w == 0)
      {
        game.snake[i-1].x = 0;
        game.snake[i-1].y = 0;
        game.snake[i-1].w = 0;
        game.snake[i-1].h = 0;
        break;
      }
    }
  }

  handle_collisions();
}

void change_direction(SDL_Keycode new_direction)
{
  bool going_up = game.dy == -CELL_HEIGHT;
  bool going_down = game.dy == CELL_HEIGHT;
  bool going_left = game.dx == -CELL_WIDTH;
  bool going_right = game.dx == CELL_WIDTH;

  if (new_direction == SDLK_UP && !going_down)
  {
    game.dx = 0;
    game.dy = -CELL_HEIGHT;
  }

  if (new_direction == SDLK_DOWN && !going_up)
  {
    game.dx = 0;
    game.dy = CELL_HEIGHT;
  }

  if (new_direction == SDLK_LEFT && !going_right)
  {
    game.dx = -CELL_WIDTH;
    game.dy = 0;
  }

  if (new_direction == SDLK_RIGHT && !going_left)
  {
    game.dx = CELL_WIDTH;
    game.dy = 0;
  }
}

void handle_collisions()
{
  // hit snake body?
  for (int i = 1; i < CELL_COUNT; i++)
  {
    if (game.snake[i].w == 0)
      break;
    if (game.snake[i].x == game.snake[0].x && game.snake[i].y == game.snake[0].y)
    {
      game.state = GAME_OVER;
      return;
    }
  }

  // hit left wall?
  if (game.snake[0].x < WALL_THICKNESS)
  {
    game.state = GAME_OVER;
    return;
  }

  // hit right wall?
  if (game.snake[0].x > SCREEN_WIDTH - WALL_THICKNESS - CELL_WIDTH)
  {
    game.state = GAME_OVER;
    return;
  }

  // hit top wall
  if (game.snake[0].y < WALL_THICKNESS)
  {
    game.state = GAME_OVER;
    return;
  }

  // hit bottom wall
  if (game.snake[0].y > SCREEN_HEIGHT - WALL_THICKNESS - CELL_HEIGHT)
  {
    game.state = GAME_OVER;
    return;
  }
}

void spawn_food()
{
  // make the food spawn in random position
  srand(time(NULL));
  game.food.x = (rand() % ((SCREEN_WIDTH - WALL_THICKNESS - CELL_WIDTH)/CELL_WIDTH+1) * CELL_WIDTH);
  game.food.y = (rand() % ((SCREEN_HEIGHT - WALL_THICKNESS - CELL_HEIGHT)/CELL_HEIGHT+1) * CELL_HEIGHT);

  // if the random number generated is less than the thickness of the left wall
  // make the food spawn next to the left wall
  if (game.food.x < WALL_THICKNESS)
    game.food.x = WALL_THICKNESS;

  // if the random number generated is less than the thickness of the top wall
  // make the food spawn next to the top wall
  if (game.food.y < WALL_THICKNESS)
    game.food.y = WALL_THICKNESS;

  // if the generated food touch the snake
  // make it spawn again
  for (int i = 0; i < CELL_COUNT; i++)
  {
    if (game.snake[i].w == 0)
      break;
    if (game.snake[i].x == game.food.x && game.snake[i].y == game.food.y)
    {
      spawn_food();
      break;
    }
  }
}

void draw_food()
{
  // load apple image
  int apple_width = CELL_WIDTH;
  int apple_height = CELL_HEIGHT;

  game.apple = IMG_LoadTexture(game.renderer, "res/apple.png");
  if (!game.apple)
  {
    printf("Error: Failed to load apple image: %s\n", SDL_GetError());
    terminate(EXIT_FAILURE);
  }

  SDL_QueryTexture(game.apple, NULL, NULL, &apple_width, &apple_height);

  SDL_Rect dest = {
    .x = game.food.x,
    .y = game.food.y,
    .w = CELL_WIDTH,
    .h = CELL_HEIGHT,
  };

  SDL_RenderCopy(game.renderer, game.apple, NULL, &dest);
}


void play_again()
{
  game.dx = CELL_WIDTH;
  game.dy = 0;
  game.score = 0;
  game.state = PLAYING;
  spawn_snake();
  spawn_food();
}


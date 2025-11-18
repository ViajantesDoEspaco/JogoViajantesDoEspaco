#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h> // NOVO: Header para SDL_image
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h> 
#include <string.h> 

// --- DEFINIÇÕES GLOBAIS ---
#define WINDOW_WIDTH 1000
#define WINDOW_HEIGHT 600
#define FRAME_DELAY_MS 16 

#define MAX_PLAYER_PROJECTILES 50 
#define MAX_ENEMY_PROJECTILES 30 
#define MAX_ENEMIES 15
#define ENEMY_HEALTH 5 
#define PLAYER_MAX_HEALTH 10 

// --- PARALLAX DEFINITIONS ---
#define MAX_STARS 50
#define MAX_METEORS 5
#define PHASE_SPEED 2 
#define NUM_METEOR_TEXTURES 5 // Quantidade de texturas de meteoro disponíveis

// --- ENUM PARA OS ESTADOS DO JOGO ---
typedef enum {
    STATE_MENU,
    STATE_GAME,
    STATE_GAME_OVER,
    STATE_CREDITS,
    STATE_EXIT
} GameState;

// --- ESTRUTURAS ---
typedef struct {
    SDL_Rect rect;
    int speed;
    bool active;
    int invencivel; 
    int fire_timer; 
    int health; 
} Player;

typedef struct {
    SDL_Rect rect;
    int active;
    int speed;
    bool is_enemy; 
} Projectile;

typedef struct {
    SDL_Rect rect;
    int health;
    bool active;
    int fire_timer; 
} Enemy;

typedef struct {
    float x, y; 
    int size;
    int layer; 
} Star;

typedef struct {
    SDL_Rect rect;
    float speed;
    bool active;
    int texture_id; // NOVO: ID da textura do meteoro (0 a 4)
} Meteor;

// --- VARIÁVEIS GLOBAIS DE JOGO ---
GameState current_state = STATE_MENU;
Player player;
Projectile player_projectiles[MAX_PLAYER_PROJECTILES];
Projectile enemy_projectiles[MAX_ENEMY_PROJECTILES]; 
Enemy enemies[MAX_ENEMIES];
Star stars[MAX_STARS];
Meteor meteors[MAX_METEORS];
// SDL_Rect moon_rect; // REMOVIDO: Não usaremos a lua

int score = 0;

// Variáveis TTF
TTF_Font* font_small = NULL;
TTF_Font* font_medium = NULL;
TTF_Font* font_large = NULL;

const char* FONT_PATH = "DejaVuSans.ttf"; 
const char* CREDITS_TEXT = 
    "DESENVOLVEDOR: CLARISSA BERLIM\n"
    "ANO: 2025\n"
    "VIAJANTES DO ESPACO\n"
    "AGRADECIMENTOS: ...";

// Variáveis de Textura
SDL_Texture* player_texture = NULL;
SDL_Texture* enemy_texture = NULL;
SDL_Texture* meteor_textures[NUM_METEOR_TEXTURES];

// Variáveis de controle de input e movimento 
bool esqPress = false, dirPress = false, upPress = false, downPress = false;
bool rodando = true;

// Variáveis de movimento dos inimigos
int enemy_dir = 1; 
int enemy_move_timer = 0;
const int ENEMY_MOVE_DELAY = 60; 
const int ENEMY_SPEED_X = 5;
const int ENEMY_SPEED_Y = 20; 

// --- FUNÇÕES UTILITÁRIAS ---

bool colidem(SDL_Rect a, SDL_Rect b) {
    return SDL_HasIntersection(&a, &b);
}

// NOVO: Função para carregar texturas
bool load_textures(SDL_Renderer* renderer) {
    char path_buffer[100];
    
    // Player
    player_texture = IMG_LoadTexture(renderer, "assets/player.png");
    if (!player_texture) {
        printf("Falha ao carregar player.png: %s\n", IMG_GetError());
        return false;
    }

    // Inimigo
    enemy_texture = IMG_LoadTexture(renderer, "assets/inimigo1.png");
    if (!enemy_texture) {
        printf("Falha ao carregar inimigo1.png: %s\n", IMG_GetError());
        return false;
    }

    // Meteoros
    char* meteor_files[] = {
        "assets/Meteor_02.png", 
        "assets/Meteor_03.png", 
        "assets/Meteor_05.png", 
        "assets/Meteor_08.png", 
        "assets/Meteor_09.png"
    };

    for (int i = 0; i < NUM_METEOR_TEXTURES; i++) {
        meteor_textures[i] = IMG_LoadTexture(renderer, meteor_files[i]);
        if (!meteor_textures[i]) {
            printf("Falha ao carregar %s: %s\n", meteor_files[i], IMG_GetError());
            return false;
        }
    }

    return true;
}


void init_stars() {
    for (int i = 0; i < MAX_STARS; i++) {
        stars[i].x = (float)(rand() % WINDOW_WIDTH);
        stars[i].y = (float)(rand() % WINDOW_HEIGHT);
        stars[i].size = (rand() % 2) + 1; 
        stars[i].layer = (rand() % 2) + 1; 
    }
}

void init_meteors() {
    for (int i = 0; i < MAX_METEORS; i++) {
        // Reduzimos o tamanho inicial do meteoro para caber na imagem
        meteors[i].rect.w = 50; 
        meteors[i].rect.h = 50; 
        meteors[i].speed = (float)((rand() % 4) + 2); 
        meteors[i].active = false;
        meteors[i].rect.y = -(rand() % WINDOW_HEIGHT);
        meteors[i].rect.x = rand() % WINDOW_WIDTH;
        meteors[i].texture_id = rand() % NUM_METEOR_TEXTURES; // NOVO: Atribui textura aleatória
    }
}

void init_player() {
    // Ajustado o tamanho do player para caber na imagem
    player.rect.w = 60; 
    player.rect.h = 80; 
    player.rect.x = (WINDOW_WIDTH / 2) - (player.rect.w / 2);
    player.rect.y = WINDOW_HEIGHT - 100; // Movido ligeiramente para cima
    player.speed = 7;
    player.active = true;
    player.invencivel = 0;
    player.fire_timer = 0;
    player.health = PLAYER_MAX_HEALTH;
}

void init_enemies() {
    int enemy_w = 40;
    int enemy_h = 70; // Ajustado o tamanho para caber na imagem
    int padding = 15;
    
    int line_width = MAX_ENEMIES * (enemy_w + padding);
    int start_x = (WINDOW_WIDTH / 2) - (line_width / 2);
    int base_y = 50;
    
    float center_index = (float)(MAX_ENEMIES - 1) / 2.0;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].rect.w = enemy_w;
        enemies[i].rect.h = enemy_h;
        
        enemies[i].rect.x = start_x + i * (enemy_w + padding);
        
        float dist_from_center = fabsf((float)i - center_index);
        enemies[i].rect.y = base_y + (int)(dist_from_center * 15);
        
        enemies[i].health = ENEMY_HEALTH;
        enemies[i].active = true;
        enemies[i].fire_timer = rand() % 120; 
    }
}

void init_player_projectiles() {
    for (int i = 0; i < MAX_PLAYER_PROJECTILES; i++) {
        player_projectiles[i].rect.w = 4;
        player_projectiles[i].rect.h = 15; // Ligeiramente maior
        player_projectiles[i].active = 0;
        player_projectiles[i].speed = -15; 
        player_projectiles[i].is_enemy = false;
    }
}

void init_enemy_projectiles() {
    for (int i = 0; i < MAX_ENEMY_PROJECTILES; i++) {
        enemy_projectiles[i].rect.w = 5;
        enemy_projectiles[i].rect.h = 20; // Ligeiramente maior
        enemy_projectiles[i].active = 0;
        enemy_projectiles[i].speed = 5; 
        enemy_projectiles[i].is_enemy = true;
    }
}

void reset_game() {
    init_player();
    init_enemies();
    init_player_projectiles();
    init_enemy_projectiles();
    init_stars();
    init_meteors();
    score = 0;
    enemy_dir = 1;
    enemy_move_timer = 0;
}

void fire_player_projectile() {
    for (int i = 0; i < MAX_PLAYER_PROJECTILES; i++) {
        if (!player_projectiles[i].active) {
            player_projectiles[i].rect.x = player.rect.x + (player.rect.w / 2) - (player_projectiles[i].rect.w / 2);
            player_projectiles[i].rect.y = player.rect.y;
            player_projectiles[i].active = 1;
            return;
        }
    }
}

void fire_enemy_projectile(int x, int y) {
    for (int i = 0; i < MAX_ENEMY_PROJECTILES; i++) {
        if (!enemy_projectiles[i].active) {
            enemy_projectiles[i].rect.x = x - (enemy_projectiles[i].rect.w / 2);
            enemy_projectiles[i].rect.y = y;
            enemy_projectiles[i].active = 1;
            return;
        }
    }
}

void update_parallax() {
    for (int i = 0; i < MAX_STARS; i++) {
        float speed = (stars[i].layer == 1) ? (float)PHASE_SPEED / 3.0f : (float)PHASE_SPEED;
        stars[i].y += speed;
        if (stars[i].y > WINDOW_HEIGHT) {
            stars[i].y = 0; 
            stars[i].x = (float)(rand() % WINDOW_WIDTH); 
        }
    }
    
    for (int i = 0; i < MAX_METEORS; i++) {
        if (meteors[i].active) {
            meteors[i].rect.y += meteors[i].speed;
            if (meteors[i].rect.y > WINDOW_HEIGHT) {
                meteors[i].active = false;
            }
            
            // Colisão do Meteoro com o Player
            if (colidem(meteors[i].rect, player.rect) && player.invencivel == 0) {
                meteors[i].active = false; // Meteoro destrói
                player.health--; 
                player.invencivel = 60; 
            }
            
        } else {
            if (rand() % 500 == 0) {
                meteors[i].active = true;
                meteors[i].rect.y = -meteors[i].rect.h;
                meteors[i].rect.x = rand() % WINDOW_WIDTH;
                meteors[i].texture_id = rand() % NUM_METEOR_TEXTURES; // Nova textura ao reaparecer
            }
        }
    }
}

void update_player() {
    if (player.health <= 0) {
        current_state = STATE_GAME_OVER;
        return;
    }

    int movimentoX = 0;
    int movimentoY = 0;

    if (esqPress) movimentoX -= player.speed;
    if (dirPress) movimentoX += player.speed;
    
    if (upPress) movimentoY -= player.speed;
    if (downPress) movimentoY += player.speed;

    player.rect.x += movimentoX;
    player.rect.y += movimentoY;

    if (player.rect.x < 0) player.rect.x = 0;
    if (player.rect.x + player.rect.w > WINDOW_WIDTH) player.rect.x = WINDOW_WIDTH - player.rect.w;

    if (player.rect.y < 0) player.rect.y = 0;
    
    if (player.rect.y + player.rect.h > WINDOW_HEIGHT) {
        player.rect.y = WINDOW_HEIGHT - player.rect.h;
    }
    
    player.rect.y += PHASE_SPEED;

    if (player.rect.y + player.rect.h > WINDOW_HEIGHT) {
        player.rect.y = WINDOW_HEIGHT - player.rect.h;
    }
    
    if (player.invencivel > 0) {
        player.invencivel--;
    }

    player.fire_timer++;
    if (player.fire_timer >= 10) { 
        fire_player_projectile();
        player.fire_timer = 0;
    }
}

void update_player_projectiles() {
    for (int i = 0; i < MAX_PLAYER_PROJECTILES; i++) {
        if (player_projectiles[i].active) {
            player_projectiles[i].rect.y += player_projectiles[i].speed;
            
            player_projectiles[i].rect.y += PHASE_SPEED; 

            if (player_projectiles[i].rect.y < -player_projectiles[i].rect.h || player_projectiles[i].rect.y > WINDOW_HEIGHT) {
                player_projectiles[i].active = 0;
            }
        }
    }
}

void update_enemy_projectiles() {
    for (int i = 0; i < MAX_ENEMY_PROJECTILES; i++) {
        if (enemy_projectiles[i].active) {
            enemy_projectiles[i].rect.y += enemy_projectiles[i].speed;
            
            enemy_projectiles[i].rect.y += PHASE_SPEED; 

            if (enemy_projectiles[i].rect.y > WINDOW_HEIGHT) {
                enemy_projectiles[i].active = 0;
            }
            
            if (colidem(enemy_projectiles[i].rect, player.rect) && player.invencivel == 0) {
                enemy_projectiles[i].active = 0;
                player.health--; 
                player.invencivel = 60;
            }
        }
    }
}

void update_enemies() {
    bool edge_hit = false;
    
    enemy_move_timer++;
    
    if (enemy_move_timer >= ENEMY_MOVE_DELAY) {
        
        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (enemies[j].active) {
                if (enemies[j].rect.x + enemies[j].rect.w + ENEMY_SPEED_X * enemy_dir > WINDOW_WIDTH) {
                    edge_hit = true;
                }
                if (enemies[j].rect.x + ENEMY_SPEED_X * enemy_dir < 0) {
                    edge_hit = true;
                }
            }
        }

        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (enemies[j].active) {
                if (edge_hit) {
                    enemies[j].rect.y += ENEMY_SPEED_Y; 
                } else {
                    enemies[j].rect.x += ENEMY_SPEED_X * enemy_dir; 
                }
            }
        }

        if (edge_hit) {
            enemy_dir *= -1; 
        }
        enemy_move_timer = 0;
    }
    
    for (int j = 0; j < MAX_ENEMIES; j++) {
        if (enemies[j].active) {
            // Colisão com o Player
            if (colidem(enemies[j].rect, player.rect) && player.invencivel == 0) {
                player.health--; 
                player.invencivel = 60; 
                enemies[j].active = false;
            }
            
            // Inimigo atira
            enemies[j].fire_timer++;
            if (enemies[j].fire_timer >= 180 + (rand() % 60)) { 
                fire_enemy_projectile(enemies[j].rect.x + enemies[j].rect.w / 2, enemies[j].rect.y + enemies[j].rect.h);
                enemies[j].fire_timer = 0;
            }
        }
    }

    // Colisão (Player Projectile vs. Enemy)
    for (int i = 0; i < MAX_PLAYER_PROJECTILES; i++) {
        if (player_projectiles[i].active) {
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (enemies[j].active && colidem(player_projectiles[i].rect, enemies[j].rect)) {
                    player_projectiles[i].active = 0;
                    enemies[j].health--;

                    if (enemies[j].health <= 0) {
                        enemies[j].active = false;
                        score += 10;
                    }
                    break; 
                }
            }
        }
    }
    
    // Checagem de Game Over (Inimigo atinge o limite inferior)
    for (int j = 0; j < MAX_ENEMIES; j++) {
        if (enemies[j].active && enemies[j].rect.y > WINDOW_HEIGHT) {
            current_state = STATE_GAME_OVER;
        }
    }
}

// --- LÓGICA DE RENDERIZAÇÃO ---

void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color) {
    if (!font) return;
    
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) {
        return;
    }
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }
    
    SDL_Rect dstRect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dstRect);
    
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void render_parallax(SDL_Renderer* renderer) {
    
    // REMOVIDO: Lógica de renderização da lua
    
    // Estrelas
    for (int i = 0; i < MAX_STARS; i++) {
        if (stars[i].layer == 1) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255); 
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); 
        }
        
        SDL_Rect star_rect = {(int)stars[i].x, (int)stars[i].y, stars[i].size, stars[i].size};
        SDL_RenderFillRect(renderer, &star_rect);
    }
    
    // Meteoros (AGORA USANDO TEXTURA)
    for (int i = 0; i < MAX_METEORS; i++) {
        if (meteors[i].active && meteor_textures[meteors[i].texture_id] != NULL) {
            SDL_RenderCopy(renderer, meteor_textures[meteors[i].texture_id], NULL, &meteors[i].rect);
        }
    }
}

void render_menu(SDL_Renderer* renderer) {
    render_parallax(renderer); 
    SDL_Color white = {255, 255, 255, 255};
    
    render_text(renderer, font_medium, "VIAJANTES DO ESPACO", WINDOW_WIDTH/2 - 250, WINDOW_HEIGHT/4, white);
    render_text(renderer, font_small, "S - INICIAR JOGO", WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 - 50, white);
    render_text(renderer, font_small, "C - CREDITOS", WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 + 20, white);
    render_text(renderer, font_small, "Q - SAIR", WINDOW_WIDTH/2 - 100, WINDOW_HEIGHT/2 + 90, white);
}

void render_game(SDL_Renderer* renderer) {
    render_parallax(renderer);
    SDL_Color white = {255, 255, 255, 255};

    // 1. Nave do Jogador (AGORA USANDO TEXTURA)
    if (player_texture != NULL && (player.invencivel == 0 || player.invencivel % 10 < 5)) {
        SDL_RenderCopy(renderer, player_texture, NULL, &player.rect);
    }
    
    // 2. Projéteis do Jogador (Amarelos)
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); 
    for (int i = 0; i < MAX_PLAYER_PROJECTILES; i++) {
        if (player_projectiles[i].active) {
            SDL_RenderFillRect(renderer, &player_projectiles[i].rect);
        }
    }
    
    // 3. Projéteis do Inimigo (Roxos/Magenta)
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); 
    for (int i = 0; i < MAX_ENEMY_PROJECTILES; i++) {
        if (enemy_projectiles[i].active) {
            SDL_RenderFillRect(renderer, &enemy_projectiles[i].rect);
        }
    }

    // 4. Inimigos (AGORA USANDO TEXTURA)
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].active && enemy_texture != NULL) {
            // Se invencibilidade do player fosse um feedback visual, poderia ser aqui
            SDL_RenderCopy(renderer, enemy_texture, NULL, &enemies[i].rect);
        }
    }

    // 5. Renderização do Score (TTF)
    char score_str[50];
    sprintf(score_str, "PONTUACAO: %d", score);
    render_text(renderer, font_small, score_str, 10, 10, white);

    // 6. Renderização da Vida do Jogador (TTF)
    char health_str[20];
    sprintf(health_str, "VIDA: %d/%d", player.health, PLAYER_MAX_HEALTH);
    
    int text_w, text_h;
    TTF_SizeText(font_small, health_str, &text_w, &text_h);
    render_text(renderer, font_small, health_str, WINDOW_WIDTH - text_w - 10, 10, white);
}

void render_game_over(SDL_Renderer* renderer) {
    render_parallax(renderer);
    SDL_Color red = {255, 0, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    
    render_text(renderer, font_medium, "GAME OVER", WINDOW_WIDTH/2 - 150, WINDOW_HEIGHT/2 - 80, red);

    char score_str[50];
    sprintf(score_str, "PONTUACAO FINAL: %d", score);
    int text_w, text_h;
    TTF_SizeText(font_small, score_str, &text_w, &text_h);
    render_text(renderer, font_small, score_str, WINDOW_WIDTH/2 - text_w/2, WINDOW_HEIGHT/2 + 20, white);

    render_text(renderer, font_small, "Pressione M para voltar ao Menu", WINDOW_WIDTH/2 - 150, WINDOW_HEIGHT/2 + 100, white);
}

void render_credits(SDL_Renderer* renderer) {
    render_parallax(renderer);
    SDL_Color white = {255, 255, 255, 255};
    
    render_text(renderer, font_medium, "CREDITOS", WINDOW_WIDTH/2 - 150, WINDOW_HEIGHT/4 - 50, white);
    
    char temp_str[256];
    strcpy(temp_str, CREDITS_TEXT);
    char* line = strtok(temp_str, "\n");
    int y_offset = WINDOW_HEIGHT/4 + 50;
    
    while(line != NULL) {
        int text_w, text_h;
        TTF_SizeText(font_small, line, &text_w, &text_h);
        render_text(renderer, font_small, line, WINDOW_WIDTH/2 - text_w/2, y_offset, white);
        y_offset += 40;
        line = strtok(NULL, "\n");
    }

    render_text(renderer, font_small, "Pressione ESC para voltar ao Menu", WINDOW_WIDTH/2 - 150, WINDOW_HEIGHT - 50, white);
}

// --- FUNÇÕES DE INPUT ---

void handle_menu_input(SDL_Event *e) {
    if (e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.scancode) {
            case SDL_SCANCODE_S:
                reset_game();
                current_state = STATE_GAME;
                break;
            case SDL_SCANCODE_Q:
                current_state = STATE_EXIT;
                break;
            case SDL_SCANCODE_C:
                current_state = STATE_CREDITS;
                break;
        }
    }
}

void handle_game_input(SDL_Event *e) {
    if (e->type == SDL_KEYDOWN && !e->key.repeat) {
        switch (e->key.keysym.scancode) {
            case SDL_SCANCODE_LEFT: esqPress = true; break;
            case SDL_SCANCODE_RIGHT: dirPress = true; break;
            case SDL_SCANCODE_UP: upPress = true; break;
            case SDL_SCANCODE_DOWN: downPress = true; break;
        }
    } else if (e->type == SDL_KEYUP) {
        switch (e->key.keysym.scancode) {
            case SDL_SCANCODE_LEFT: esqPress = false; break;
            case SDL_SCANCODE_RIGHT: dirPress = false; break;
            case SDL_SCANCODE_UP: upPress = false; break;
            case SDL_SCANCODE_DOWN: downPress = false; break;
        }
    }
}

// --- MAIN LOOP ---

int main(int argc, char **argv) {
    srand((unsigned int)SDL_GetTicks()); 
    
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Erro ao inicializar SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    if (TTF_Init() == -1) {
        printf("Erro ao inicializar SDL_ttf: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    
    // NOVO: Inicialização do SDL_image
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("Erro ao inicializar SDL_image: %s\n", IMG_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    font_small = TTF_OpenFont(FONT_PATH, 18);
    font_medium = TTF_OpenFont(FONT_PATH, 30);
    font_large = TTF_OpenFont(FONT_PATH, 60);

    if (!font_small || !font_medium || !font_large) {
        printf("Erro ao carregar fonte (%s): %s\n", FONT_PATH, TTF_GetError());
    }

    SDL_Window* window = SDL_CreateWindow("VIAJANTES DO ESPACO",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Erro ao criar renderer: %s\n", SDL_GetError());
        goto cleanup;
    }

    // NOVO: Carregar todas as texturas
    if (!load_textures(renderer)) {
        printf("Erro fatal ao carregar texturas.\n");
        goto cleanup;
    }

    reset_game(); 
    
    SDL_Event e;
    
    while (rodando) {
        
        if (SDL_WaitEventTimeout(&e, FRAME_DELAY_MS)) { 
            if (e.type == SDL_QUIT) {
                rodando = false;
                current_state = STATE_EXIT;
            }
            
            switch (current_state) {
                case STATE_MENU:
                    handle_menu_input(&e);
                    break;
                case STATE_GAME:
                    handle_game_input(&e);
                    break;
                case STATE_GAME_OVER:
                    if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_M) {
                        current_state = STATE_MENU;
                    }
                    break;
                case STATE_CREDITS:
                    if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        current_state = STATE_MENU;
                    }
                    break;
                default:
                    break;
            }
        }
        
        if (current_state == STATE_EXIT) break;

        // 2. ATUALIZAÇÃO
        if (current_state == STATE_GAME) {
            update_parallax();
            update_player();
            update_player_projectiles();
            update_enemy_projectiles();
            update_enemies();
        } else if (current_state == STATE_MENU || current_state == STATE_GAME_OVER || current_state == STATE_CREDITS) {
            update_parallax();
        }

        // 3. RENDERIZAÇÃO
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
        SDL_RenderClear(renderer);
        
        switch (current_state) {
            case STATE_MENU:
                render_menu(renderer);
                break;
            case STATE_GAME:
                render_game(renderer);
                break;
            case STATE_GAME_OVER:
                render_game_over(renderer);
                break;
            case STATE_CREDITS:
                render_credits(renderer);
                break;
            default:
                break;
        }

        SDL_RenderPresent(renderer);
    }

// --- Limpeza Final ---
cleanup:
    // Texturas
    SDL_DestroyTexture(player_texture);
    SDL_DestroyTexture(enemy_texture);
    for (int i = 0; i < NUM_METEOR_TEXTURES; i++) {
        SDL_DestroyTexture(meteor_textures[i]);
    }
    IMG_Quit(); // NOVO: Limpeza SDL_image
    
    // TTF
    TTF_CloseFont(font_small);
    TTF_CloseFont(font_medium);
    TTF_CloseFont(font_large);
    TTF_Quit();
    
    // SDL
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
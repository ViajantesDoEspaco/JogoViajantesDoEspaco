#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h> 
#include <string.h> 

// --- GERAIS ---
#define WINDOW_LARGURA 1300
#define WINDOW_ALTURA 650
#define DELAY_FRAME 16 

#define PLAYER_MAX_TIROS 50 
#define INIMIGOS_MAX_TIROS 30 
#define MAX_INIMIGOS 15
#define SAUDE_INIMIGOS 5 
#define PLAYER_MAX_SAUDE 10 

// --- PARALLAX ---
#define MAX_ESTRELAS 50
#define MAX_METEOROS 5
#define VELOCIDADE_FASE 2 
#define TEXTURAS_METEOROS 5

// --- ENUM PARA OS ESTADOS DO JOGO ---
typedef enum {
    MENU,
    JOGANDO,
    GAME_OVER,
    CREDITOS,
    SAIR
} EstadoJogo;

// --- ESTRUTURAS ---
typedef struct {
    SDL_Rect rect;
    int velocidade;
    bool ativo;
    int invencivel; 
    int timer_tiros; 
    int saude; 
} Player;

typedef struct {
    SDL_Rect rect;
    int ativo;
    int velocidade;
    bool eh_inimigo; 
} Projetil;

typedef struct {
    SDL_Rect rect;
    int saude;
    bool ativo;
    int timer_tiros; 
} Inimigo;

typedef struct {
    float x, y; 
    int tamanho;
    int camada; 
} Estrela;

typedef struct {
    SDL_Rect rect;
    float velocidade;
    bool ativo;
    int id_textura; // ID da textura do meteoro (0 a 4)
} Meteoro;

// --- VARIÁVEIS GLOBAIS ---
EstadoJogo estado_atual = MENU;
Player player;
Projetil player_tiros[PLAYER_MAX_TIROS];
Projetil inimigo_tiros[INIMIGOS_MAX_TIROS]; 
Inimigo inimigos[MAX_INIMIGOS];
Estrela estrelas[MAX_ESTRELAS];
Meteoro meteoros[MAX_METEOROS];
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

int pontuacao = 0;

//TTF
TTF_Font* font_smaller = NULL;
TTF_Font* font_small = NULL;
TTF_Font* font_medium = NULL;
TTF_Font* font_large = NULL;

const char* FONT_PATH = "PressStart.ttf"; 

SDL_Texture* player_textura = NULL;
SDL_Texture* inimigo_textura = NULL;
SDL_Texture* meteoro_textura[TEXTURAS_METEOROS];

// Variáveis de controle de movimento 
bool esqPress = false;
bool dirPress = false;
bool cimaPress = false;
bool baixoPress = false;

bool rodando = true;

// Variáveis de movimento dos inimigos
int inimigo_dir = 1; 
int inimigo_mover_timer = 0;
const int inimigo_mover_delay = 60; 
const int inimigo_velX= 5;
const int inimigo_velY = 20;

// --- FUNÇÕES ---

bool colidem(SDL_Rect a, SDL_Rect b) {
    return SDL_HasIntersection(&a, &b);
}

// Carregar texturas
bool carregar_texturas(SDL_Renderer* renderer) {
    char buffer[100];
    
    // Player
    player_textura = IMG_LoadTexture(renderer, "assets/player.png");
    if (!player_textura) {
        printf("Falha ao carregar player.png: %s\n", IMG_GetError());
        return false;
    }

    // Inimigo
    inimigo_textura = IMG_LoadTexture(renderer, "assets/inimigo1.png");
    if (!inimigo_textura) {
        printf("Falha ao carregar inimigo1.png: %s\n", IMG_GetError());
        return false;
    }

    // Meteoros
    char* arquivos_meteoros[] = {
        "assets/Meteor_02.png", 
        "assets/Meteor_03.png", 
        "assets/Meteor_05.png", 
        "assets/Meteor_08.png", 
        "assets/Meteor_09.png"
    };

    for (int i = 0; i < TEXTURAS_METEOROS; i++) {
        meteoro_textura[i] = IMG_LoadTexture(renderer, arquivos_meteoros[i]);
        if (!meteoro_textura[i]) {
            printf("Falha ao carregar %s: %s\n", arquivos_meteoros[i], IMG_GetError());
            return false;
        }
    }

    return true;
}

void iniciar_estrelas() {
    for (int i = 0; i < MAX_ESTRELAS; i++) {
        estrelas[i].x = (float)(rand() % WINDOW_LARGURA);
        estrelas[i].y = (float)(rand() % WINDOW_ALTURA);
        estrelas[i].tamanho = (rand() % 2) + 1; 
        estrelas[i].camada = (rand() % 2) + 1; 
    }
}

void iniciar_meteoros() {
    for (int i = 0; i < MAX_METEOROS; i++) {
        meteoros[i].rect.w = 50; 
        meteoros[i].rect.h = 50; 
        meteoros[i].velocidade = (float)((rand() % 4) + 2); 
        meteoros[i].ativo = false;
        meteoros[i].rect.y = -(rand() % WINDOW_ALTURA);
        meteoros[i].rect.x = rand() % WINDOW_LARGURA;
        meteoros[i].id_textura = rand() % TEXTURAS_METEOROS;
    }
}

void iniciar_player() {
    player.rect.w = 60; 
    player.rect.h = 80; 
    player.rect.x = (WINDOW_LARGURA / 2) - (player.rect.w / 2);
    player.rect.y = WINDOW_ALTURA - 100;
    player.velocidade = 7;
    player.ativo = true;
    player.invencivel = 0;
    player.timer_tiros = 0;
    player.saude = PLAYER_MAX_SAUDE;
}

void iniciar_inimigos() {
    int inimigo_l = 40;
    int inimigo_h = 70;
    int afastamento = 15;
    
    int largura_linha = MAX_INIMIGOS * (inimigo_l + afastamento);
    int x_inicio = (WINDOW_LARGURA / 2) - (largura_linha / 2);
    int y_inicio = 50;
    
    float indice_inicial = (float)(MAX_INIMIGOS - 1) / 2.0;

    for (int i = 0; i < MAX_INIMIGOS; i++) {
        inimigos[i].rect.w = inimigo_l;
        inimigos[i].rect.h = inimigo_h;
        
        inimigos[i].rect.x = x_inicio + i * (inimigo_l + afastamento);
        
        float dist_do_centro = fabsf((float)i - indice_inicial);
        inimigos[i].rect.y = y_inicio + (int)(dist_do_centro * 15);
        
        inimigos[i].saude = SAUDE_INIMIGOS;
        inimigos[i].ativo = true;
        inimigos[i].timer_tiros = rand() % 120;
    }
}

void iniciar_player_tiros() {
    for (int i = 0; i < PLAYER_MAX_TIROS; i++) {
        player_tiros[i].rect.w = 4;
        player_tiros[i].rect.h = 15;
        player_tiros[i].ativo = 0;
        player_tiros[i].velocidade = -15;
        player_tiros[i].eh_inimigo = false;
    }
}

void iniciar_inimigos_tiros() {
    for (int i = 0; i < INIMIGOS_MAX_TIROS; i++) {
        inimigo_tiros[i].rect.w = 5;
        inimigo_tiros[i].rect.h = 20;
        inimigo_tiros[i].ativo = 0;
        inimigo_tiros[i].velocidade = 5; 
        inimigo_tiros[i].eh_inimigo = true;
    }
}

void resetar_jogo() {
    iniciar_player();
    iniciar_inimigos();
    iniciar_player_tiros();
    iniciar_inimigos_tiros();
    iniciar_estrelas();
    iniciar_meteoros();
    pontuacao = 0;
    inimigo_dir = 1;
    inimigo_mover_timer = 0;
}

void player_atirar() {
    for (int i = 0; i < PLAYER_MAX_TIROS; i++) {
        if (!player_tiros[i].ativo) {
            player_tiros[i].rect.x = player.rect.x + (player.rect.w / 2) - (player_tiros[i].rect.w / 2);
            player_tiros[i].rect.y = player.rect.y;
            player_tiros[i].ativo = 1;
            return;
        }
    }
}

void inimigo_atirar(int x, int y) {
    for (int i = 0; i < INIMIGOS_MAX_TIROS; i++) {
        if (!inimigo_tiros[i].ativo) {
            inimigo_tiros[i].rect.x = x - (inimigo_tiros[i].rect.w / 2);
            inimigo_tiros[i].rect.y = y;
            inimigo_tiros[i].ativo = 1;
            return;
        }
    }
}

void update_parallax() {
    for (int i = 0; i < MAX_ESTRELAS; i++) {
        float velocidade = (estrelas[i].camada == 1) ? (float)VELOCIDADE_FASE / 3.0f : (float)VELOCIDADE_FASE;
        estrelas[i].y += velocidade;
        if (estrelas[i].y > WINDOW_ALTURA) {
            estrelas[i].y = 0; 
            estrelas[i].x = (float)(rand() % WINDOW_LARGURA);
        }
    }
    
    for (int i = 0; i < MAX_METEOROS; i++) {
        if (meteoros[i].ativo) {
            meteoros[i].rect.y += meteoros[i].velocidade;
            if (meteoros[i].rect.y > WINDOW_ALTURA) {
                meteoros[i].ativo = false;
            }
            
            // Colisão do Meteoro com o player
            if (colidem(meteoros[i].rect, player.rect) && player.invencivel == 0) {
                meteoros[i].ativo = false; // Meteoro destrói
                player.saude--; 
                player.invencivel = 60; 
            }
            
        } else {
            if (rand() % 500 == 0) {
                meteoros[i].ativo = true;
                meteoros[i].rect.y = -meteoros[i].rect.h;
                meteoros[i].rect.x = rand() % WINDOW_LARGURA;
                meteoros[i].id_textura = rand() % TEXTURAS_METEOROS; // Nova textura ao reaparecer
            }
        }
    }
}

void update_player() {
    if (player.saude <= 0) {
        estado_atual = GAME_OVER;
        return;
    }
    int movimentoX = 0;
    int movimentoY = 0;

    if (esqPress) movimentoX -= player.velocidade;
    if (dirPress) movimentoX += player.velocidade;
    
    if (cimaPress) movimentoY -= player.velocidade;
    if (baixoPress) movimentoY += player.velocidade;

    player.rect.x += movimentoX;
    player.rect.y += movimentoY;

    //Definições para limitar o player aos limites da tela
    if (player.rect.x < 0) player.rect.x = 0;
    if (player.rect.x + player.rect.w > WINDOW_LARGURA) {
        player.rect.x = WINDOW_LARGURA - player.rect.w;
    }

    if (player.rect.y < 0) player.rect.y = 0;
    if (player.rect.y + player.rect.h > WINDOW_ALTURA) {
        player.rect.y = WINDOW_ALTURA - player.rect.h;
    }
    
    player.rect.y += VELOCIDADE_FASE;

    if (player.rect.y + player.rect.h > WINDOW_ALTURA) {
        player.rect.y = WINDOW_ALTURA - player.rect.h;
    }
    
    //Caso atingido
    if (player.invencivel > 0) {
        player.invencivel--;
    }

    player.timer_tiros++;
    if (player.timer_tiros >= 10) { 
        player_atirar();
        player.timer_tiros = 0;
    }
}

void update_player_tiros() {
    for (int i = 0; i < PLAYER_MAX_TIROS; i++) {
        if (player_tiros[i].ativo) {
            player_tiros[i].rect.y += player_tiros[i].velocidade;
            player_tiros[i].rect.y += VELOCIDADE_FASE; 

            if (player_tiros[i].rect.y < -player_tiros[i].rect.h || player_tiros[i].rect.y > WINDOW_ALTURA) {
                player_tiros[i].ativo = 0;
            }
        }
    }
}

void update_inimigo_tiros() {
    for (int i = 0; i < INIMIGOS_MAX_TIROS; i++) {
        if (inimigo_tiros[i].ativo) {
            inimigo_tiros[i].rect.y += inimigo_tiros[i].velocidade;
            
            inimigo_tiros[i].rect.y += VELOCIDADE_FASE; 

            if (inimigo_tiros[i].rect.y > WINDOW_ALTURA) {
                inimigo_tiros[i].ativo = 0;
            }
            
            //Colisão com o player
            if (colidem(inimigo_tiros[i].rect, player.rect) && player.invencivel == 0) {
                inimigo_tiros[i].ativo = 0;
                player.saude--; 
                player.invencivel = 60;
            }
        }
    }
}

void update_inimigos() {
    bool atingiu_limite = false;
    inimigo_mover_timer++;
    
    if (inimigo_mover_timer >= inimigo_mover_delay) {
        for (int j = 0; j < MAX_INIMIGOS; j++) {
            if (inimigos[j].ativo) {
                if (inimigos[j].rect.x + inimigos[j].rect.w +
                    inimigo_velX * inimigo_dir > WINDOW_LARGURA) {
                    atingiu_limite = true;
                }
                if (inimigos[j].rect.x + inimigo_velX * inimigo_dir < 0) {
                    atingiu_limite = true;
                }
            }
        }

        for (int j = 0; j < MAX_INIMIGOS; j++) {
            if (inimigos[j].ativo) {
                if (atingiu_limite) {
                    inimigos[j].rect.y += inimigo_velY; 
                } else {
                    inimigos[j].rect.x += inimigo_velX * inimigo_dir; 
                }
            }
        }

        if (atingiu_limite) {
            inimigo_dir *= -1; 
        }
        inimigo_mover_timer = 0;
    }
    
    for (int j = 0; j < MAX_INIMIGOS; j++) {
        if (inimigos[j].ativo) {
            // Colisão com o player
            if (colidem(inimigos[j].rect, player.rect) && player.invencivel == 0) {
                player.saude--; 
                player.invencivel = 60; 
                inimigos[j].ativo = false;
            }
            
            // Inimigo atira
            inimigos[j].timer_tiros++;
            if (inimigos[j].timer_tiros >= 180 + (rand() % 60)) { 
                inimigo_atirar(inimigos[j].rect.x + inimigos[j].rect.w / 2,
                                inimigos[j].rect.y + inimigos[j].rect.h);
                inimigos[j].timer_tiros = 0;
            }
        }
    }

    // Colisão (Tiros do player vs. inimigo)
    for (int i = 0; i < PLAYER_MAX_TIROS; i++) {
        if (player_tiros[i].ativo) {
            for (int j = 0; j < MAX_INIMIGOS; j++) {
                if (inimigos[j].ativo && colidem(player_tiros[i].rect, inimigos[j].rect)) {
                    player_tiros[i].ativo = 0;
                    inimigos[j].saude--;

                    if (inimigos[j].saude <= 0) {
                        inimigos[j].ativo = false;
                        pontuacao += 10;
                    }
                    break; 
                }
            }
        }
    }
    
    // Checagem de Game Over (Inimigo atinge o limite inferior)
    for (int j = 0; j < MAX_INIMIGOS; j++) {
        if (inimigos[j].ativo && inimigos[j].rect.y > WINDOW_ALTURA) {
            estado_atual = GAME_OVER;
        }
    }
}

// --- RENDERIZAÇÃO ---

void render_texto(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color) {
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
    // Estrelas
    for (int i = 0; i < MAX_ESTRELAS; i++) {
        if (estrelas[i].camada == 1) {
            SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255); 
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); 
        }
        
        SDL_Rect estrela_rect = {(int)estrelas[i].x, (int)estrelas[i].y, estrelas[i].tamanho, estrelas[i].tamanho};
        SDL_RenderFillRect(renderer, &estrela_rect);
    }
    
    // Meteoros
    for (int i = 0; i < MAX_METEOROS; i++) {
        if (meteoros[i].ativo && meteoro_textura[meteoros[i].id_textura] != NULL) {
            SDL_RenderCopy(renderer, meteoro_textura[meteoros[i].id_textura], NULL, &meteoros[i].rect);
        }
    }
}

void render_menu(SDL_Renderer* renderer) {
    render_parallax(renderer); 
    SDL_Color white = {255, 255, 255, 255};
    
    render_texto(renderer, font_medium, "VIAJANTES DO ESPACO", WINDOW_LARGURA/2 - 250, WINDOW_ALTURA/4, white);
    render_texto(renderer, font_small, "INICIAR JOGO", WINDOW_LARGURA/2 - 100, WINDOW_ALTURA/2 - 50, white);
    render_texto(renderer, font_small, "CREDITOS", WINDOW_LARGURA/2 - 100, WINDOW_ALTURA/2 + 20, white);
    render_texto(renderer, font_small, "SAIR", WINDOW_LARGURA/2 - 100, WINDOW_ALTURA/2 + 90, white);
    
    render_texto(renderer, font_smaller, "Pressione S para iniciar, C para ver os creditos e Q para sair do jogo", WINDOW_LARGURA/2 - 450, WINDOW_ALTURA - 50, white);
}

void render_jogo(SDL_Renderer* renderer) {
    render_parallax(renderer);
    SDL_Color white = {255, 255, 255, 255};

    // 1. Nave do Jogador (textura)
    if (player_textura != NULL && (player.invencivel == 0 || player.invencivel % 10 < 5)) {
        SDL_RenderCopy(renderer, player_textura, NULL, &player.rect);
    }
    
    // 2. Projéteis do Jogador (Amarelos)
    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); 
    for (int i = 0; i < PLAYER_MAX_TIROS; i++) {
        if (player_tiros[i].ativo) {
            SDL_RenderFillRect(renderer, &player_tiros[i].rect);
        }
    }
    
    // 3. Projéteis do Inimigo (Rosa)
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255); 
    for (int i = 0; i < INIMIGOS_MAX_TIROS; i++) {
        if (inimigo_tiros[i].ativo) {
            SDL_RenderFillRect(renderer, &inimigo_tiros[i].rect);
        }
    }

    // 4. Inimigos (textura)
    for (int i = 0; i < MAX_INIMIGOS; i++) {
        if (inimigos[i].ativo && inimigo_textura != NULL) {
            // Implementar invencibilidade visual depois
            SDL_RenderCopy(renderer, inimigo_textura, NULL, &inimigos[i].rect);
        }
    }

    // 5. Renderização do pontuacao (TTF)
    char pontuacao_str[50];
    sprintf(pontuacao_str, "PONTUACAO: %d", pontuacao);
    render_texto(renderer, font_smaller, pontuacao_str, 10, 10, white);

    // 6. Renderização da Vida do Jogador (TTF)
    char saude_str[20];
    sprintf(saude_str, "VIDA: %d/%d", player.saude, PLAYER_MAX_SAUDE);
    
    int text_w, text_h;
    TTF_SizeText(font_smaller, saude_str, &text_w, &text_h);
    render_texto(renderer, font_smaller, saude_str, WINDOW_LARGURA - text_w - 10, 10, white);
}

void render_game_over(SDL_Renderer* renderer) {
    render_parallax(renderer);
    SDL_Color red = {255, 0, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    
    render_texto(renderer, font_medium, "GAME OVER", WINDOW_LARGURA/2 - 150, WINDOW_ALTURA/2 - 80, red);

    char pontuacao_str[50];
    sprintf(pontuacao_str, "PONTUACAO FINAL: %d", pontuacao);
    int text_w, text_h;
    TTF_SizeText(font_small, pontuacao_str, &text_w, &text_h);
    render_texto(renderer, font_small, pontuacao_str, WINDOW_LARGURA/2 - text_w/2, WINDOW_ALTURA/2 + 20, white);

    render_texto(renderer, font_smaller, "Pressione M para voltar ao Menu", WINDOW_LARGURA/2 - 150, WINDOW_ALTURA/2 + 100, white);
}

void render_creditos(SDL_Renderer* renderer) {
    render_parallax(renderer);
    SDL_Color white = {255, 255, 255, 255};
    
    render_texto(renderer, font_medium, "CREDITOS", WINDOW_LARGURA/2 - 100, WINDOW_ALTURA/4 - 50, white);
    
    render_texto(renderer, font_small, "Desenvolvedor: Clarissa Berlim", WINDOW_LARGURA/2 - 250, WINDOW_ALTURA/2 - 50, white);
    render_texto(renderer, font_small, "Ano: 2025", WINDOW_LARGURA/2 - 250, WINDOW_ALTURA/2 + 20, white);
    render_texto(renderer, font_small, "Agradecimentos: ....", WINDOW_LARGURA/2 - 250, WINDOW_ALTURA/2 + 90, white);

    render_texto(renderer, font_smaller, "Pressione ESC para voltar ao Menu", WINDOW_LARGURA/2 - 300, WINDOW_ALTURA - 50, white);
}


// --- FUNÇÃO LIMPEZA ---
void limpar() {
    // Texturas
    SDL_DestroyTexture(player_textura);
    SDL_DestroyTexture(inimigo_textura);
    for (int i = 0; i < TEXTURAS_METEOROS; i++) {
        SDL_DestroyTexture(meteoro_textura[i]);
    }
    IMG_Quit();
    
    // TTF
    TTF_CloseFont(font_small);
    TTF_CloseFont(font_medium);
    TTF_CloseFont(font_large);
    TTF_Quit();
    
    // SDL
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}
    
// --- FUNÇÕES DE INPUT ---

void menu_input(SDL_Event *e) {
    if (e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.scancode) {
            case SDL_SCANCODE_S:
                resetar_jogo();
                estado_atual = JOGANDO;
                break;
            case SDL_SCANCODE_Q:
                estado_atual = SAIR;
                break;
            case SDL_SCANCODE_C:
                estado_atual = CREDITOS;
                break;
        }
    }
}

void jogo_input(SDL_Event *e) {
    if (e->type == SDL_KEYDOWN && !e->key.repeat) {
        switch (e->key.keysym.scancode) {
            case SDL_SCANCODE_LEFT: esqPress = true; break;
            case SDL_SCANCODE_RIGHT: dirPress = true; break;
            case SDL_SCANCODE_UP: cimaPress = true; break;
            case SDL_SCANCODE_DOWN: baixoPress = true; break;
        }
    } else if (e->type == SDL_KEYUP) {
        switch (e->key.keysym.scancode) {
            case SDL_SCANCODE_LEFT: esqPress = false; break;
            case SDL_SCANCODE_RIGHT: dirPress = false; break;
            case SDL_SCANCODE_UP: cimaPress = false; break;
            case SDL_SCANCODE_DOWN: baixoPress = false; break;
        }
    }
}

// --- MAIN ---

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
    
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("Erro ao inicializar SDL_image: %s\n", IMG_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    font_smaller = TTF_OpenFont(FONT_PATH, 14);
    font_small = TTF_OpenFont(FONT_PATH, 18);
    font_medium = TTF_OpenFont(FONT_PATH, 30);
    font_large = TTF_OpenFont(FONT_PATH, 60);

    if (!font_smaller || !font_small || !font_medium || !font_large) {
        printf("Erro ao carregar fonte (%s): %s\n", FONT_PATH, TTF_GetError());
    }

    SDL_Window* window = SDL_CreateWindow("VIAJANTES DO ESPACO",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_LARGURA, WINDOW_ALTURA, 0);

    SDL_Renderer* renderer;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Erro ao criar renderer: %s\n", SDL_GetError());
        limpar();
    }

    // Carregar todas as texturas
    if (!carregar_texturas(renderer)) {
        printf("Erro fatal ao carregar texturas.\n");
        limpar();
    }

    resetar_jogo(); 
    
    SDL_Event e;
    
    while (rodando) {
        if (SDL_WaitEventTimeout(&e, DELAY_FRAME)) { 
            if (e.type == SDL_QUIT) {
                rodando = false;
                estado_atual = SAIR;
            }
            
            switch (estado_atual) {
                case MENU:
                    menu_input(&e);
                    break;
                case JOGANDO:
                    jogo_input(&e);
                    break;
                case GAME_OVER:
                    if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_M) {
                        estado_atual = MENU;
                    }
                case CREDITOS:
                    if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        estado_atual = MENU;
                    }
                default:
                    break;
            }
        }
        
        if (estado_atual == SAIR) break;

        // 2. ATUALIZAÇÃO
        if (estado_atual == JOGANDO) {
            update_parallax();
            update_player();
            update_player_tiros();
            update_inimigo_tiros();
            update_inimigos();
        } else if (estado_atual == MENU || estado_atual == GAME_OVER ||
                    estado_atual == CREDITOS) {
            update_parallax();
        }

        // 3. RENDERIZAÇÃO
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); 
        SDL_RenderClear(renderer);
        
        switch (estado_atual) {
            case MENU:
                render_menu(renderer);
                break;
            case JOGANDO:
                render_jogo(renderer);
                break;
            case GAME_OVER:
                render_game_over(renderer);
                break;
            case CREDITOS:
                render_creditos(renderer);
                break;
            default:
                break;
        }

        SDL_RenderPresent(renderer);
    }

    limpar();
    return 0;
}

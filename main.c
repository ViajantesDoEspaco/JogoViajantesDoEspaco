#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
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

// --- EXPLOSAO ---
#define EXPLOSAO_FRAMES 4
#define MAX_EXPLOSOES 30

// --- POWERUPS ---
#define MAX_POWERUPS 10

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

typedef struct {
    SDL_Rect rect;
    int frame;   // 0..EXPLOSAO_FRAMES-1
    int ativo;
    int timer;   // contador de frames
} Explosao;

typedef struct {
    SDL_Rect rect;
    int ativo;
    int tipo;   // 0 = hp, 1 = damage
    float velX;
    float velY;
} PowerUp;

// --- VARIÁVEIS GLOBAIS ---
EstadoJogo estado_atual = MENU;
Player player;
Projetil player_tiros[PLAYER_MAX_TIROS];
Projetil inimigo_tiros[INIMIGOS_MAX_TIROS]; 
Inimigo inimigos[MAX_INIMIGOS];
Estrela estrelas[MAX_ESTRELAS];
Meteoro meteoros[MAX_METEOROS];
Explosao explosoes[MAX_EXPLOSOES];
PowerUp powerups[MAX_POWERUPS];

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

int pontuacao = 0;

//TTF
TTF_Font* font_smaller = NULL;
TTF_Font* font_small = NULL;
TTF_Font* font_medium = NULL;
TTF_Font* font_large = NULL;

const char* FONT_PATH = "assets/PressStart.ttf"; 

SDL_Texture* player_textura = NULL;
SDL_Texture* inimigo_textura = NULL;
SDL_Texture* inimigo2_textura = NULL;
SDL_Texture* meteoro_textura[TEXTURAS_METEOROS];
SDL_Texture* explosao_texturas[EXPLOSAO_FRAMES];
SDL_Texture* powerup_hp = NULL;
SDL_Texture* powerup_damage = NULL;

// Movimento player
bool esqPress = false;
bool dirPress = false;
bool cimaPress = false;
bool baixoPress = false;

bool rodando = true;

// Movimento inimigos
int inimigo_dir = 1; 
int inimigo_mover_timer = 0;
const int inimigo_mover_delay = 60; 
const int inimigo_velX= 7;
const int inimigo_velY = 25;

// Efeitos sonoros
Mix_Music* musica_geral = NULL;
Mix_Chunk* som_tiros = NULL;
Mix_Chunk* som_gameover = NULL;
Mix_Chunk* som_explosao = NULL;
Mix_Chunk* som_powerUp = NULL;

// Controle power-ups
bool powerups_spawned = false;
int dano_do_player = 1;

// Segunda onda
bool segunda_onda_spawnada = false;
Uint32 tempo_para_segunda_onda = 0;
bool inimigos_onda2 = false;


// --- FUNÇÕES ---

bool colidem(SDL_Rect a, SDL_Rect b) {
    return SDL_HasIntersection(&a, &b);
}

// Carregar texturas
bool carregar_texturas(SDL_Renderer* renderer) {    
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

    // Inimigo 2 (segunda onda)
    inimigo2_textura = IMG_LoadTexture(renderer, "assets/inimigo2.png");
    if (!inimigo2_textura) {
        printf("Falha ao carregar inimigo2.png: %s\n", IMG_GetError());
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

    // Explosões (4 frames)
    char* arquivos_explosao[EXPLOSAO_FRAMES] = {
        "assets/explosao_1.png",
        "assets/explosao_2.png",
        "assets/explosao_3.png",
        "assets/explosao_4.png"
    };

    for (int i = 0; i < EXPLOSAO_FRAMES; i++) {
        explosao_texturas[i] = IMG_LoadTexture(renderer, arquivos_explosao[i]);
        if (!explosao_texturas[i]) {
            printf("Falha ao carregar %s: %s\n", arquivos_explosao[i], IMG_GetError());
            return false;
        }
    }

    // Powerups
    powerup_hp = IMG_LoadTexture(renderer, "assets/bonus_hp.png");
    if (!powerup_hp) {
        printf("Falha ao carregar bonus_hp.png: %s\n", IMG_GetError());
        return false;
    }
    powerup_damage = IMG_LoadTexture(renderer, "assets/bonus_damage.png");
    if (!powerup_damage) {
        printf("Falha ao carregar bonus_damage.png: %s\n", IMG_GetError());
        return false;
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

void iniciar_inimigos_onda2() {
    int inimigo_l = 45;
    int inimigo_h = 80;
    int afastamento = 20;

    int largura_linha = MAX_INIMIGOS * (inimigo_l + afastamento);
    int x_inicio = (WINDOW_LARGURA / 2) - (largura_linha / 2);

    for (int i = 0; i < MAX_INIMIGOS; i++) {
        inimigos[i].rect.w = inimigo_l;
        inimigos[i].rect.h = inimigo_h;

        inimigos[i].rect.x = x_inicio + i * (inimigo_l + afastamento);

        // começam fora da tela
        inimigos[i].rect.y = -150 - (i * 18);
        inimigos[i].saude = 12; 
        inimigos[i].ativo = true;
        inimigos[i].timer_tiros = rand() % 90;
    }

    inimigos_onda2 = true;
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

void iniciar_explosoes() {
    for (int i = 0; i < MAX_EXPLOSOES; i++) {
        explosoes[i].ativo = 0;
        explosoes[i].frame = 0;
        explosoes[i].timer = 0;
        explosoes[i].rect.x = 0;
        explosoes[i].rect.y = 0;
        explosoes[i].rect.w = 0;
        explosoes[i].rect.h = 0;
    }
}

void iniciar_powerups() {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        powerups[i].ativo = 0;
        powerups[i].tipo = 0;
        powerups[i].velX = 0.0f;
        powerups[i].velY = 0.0f;
        powerups[i].rect.w = 0;
        powerups[i].rect.h = 0;
    }
}

void resetar_jogo() {
    iniciar_player();
    iniciar_inimigos();
    iniciar_player_tiros();
    iniciar_inimigos_tiros();
    iniciar_estrelas();
    iniciar_meteoros();
    iniciar_explosoes();
    iniciar_powerups();
    pontuacao = 0;
    inimigo_dir = 1;
    inimigo_mover_timer = 0;
    dano_do_player = 1;
    powerups_spawned = false;
}

void criar_explosao_em(SDL_Rect srcRect) {
    for (int e = 0; e < MAX_EXPLOSOES; e++) {
        if (!explosoes[e].ativo) {
            explosoes[e].ativo = 1;
            explosoes[e].frame = 0;
            explosoes[e].timer = 0;
            explosoes[e].rect.x = srcRect.x;
            explosoes[e].rect.y = srcRect.y;
            explosoes[e].rect.w = 50;
            explosoes[e].rect.h = 50;
            Mix_PlayChannel(-1, som_explosao, 0);
            break;
        }
    }
}

void spawn_powerups_no(int x, int y) {
    // spawn dois powerups no local x,y
    int spawned = 0;
    for (int i = 0; i < MAX_POWERUPS && spawned < 2; i++) {
        if (!powerups[i].ativo) {
            powerups[i].ativo = 1;
            powerups[i].rect.w = 40;
            powerups[i].rect.h = 40;
            powerups[i].rect.x = x;
            powerups[i].rect.y = y;

            powerups[i].tipo = spawned; // 0 -> hp, 1 -> damage

            // direção diagonal para baixo: um para esquerda, outro para direita, velocidades diferentes
            if (spawned == 0) {
                powerups[i].velX = -1.5f;
                powerups[i].velY = 2.0f;
            } else {
                powerups[i].velX = 1.5f;
                powerups[i].velY = 1.0f;
            }
            spawned++;
        }
    }
}

void player_atirar() {
    for (int i = 0; i < PLAYER_MAX_TIROS; i++) {
        if (!player_tiros[i].ativo) {
            player_tiros[i].rect.x = player.rect.x + (player.rect.w / 2) - (player_tiros[i].rect.w / 2);
            player_tiros[i].rect.y = player.rect.y;
            player_tiros[i].ativo = 1;
            Mix_PlayChannel(-1, som_tiros, 0);
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
        Mix_PlayChannel(-1, som_gameover, 0);
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

void update_explosoes() {
    for (int i = 0; i < MAX_EXPLOSOES; i++) {
        if (explosoes[i].ativo) {
            explosoes[i].timer++;
            if (explosoes[i].timer >= 6) { // frames por troca
                explosoes[i].timer = 0;
                explosoes[i].frame++;
                if (explosoes[i].frame >= EXPLOSAO_FRAMES) {
                    explosoes[i].ativo = 0;
                }
            }
        }
    }
}

void update_powerups() {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (powerups[i].ativo) {
            powerups[i].rect.x = (int)(powerups[i].rect.x + powerups[i].velX);
            powerups[i].rect.y = (int)(powerups[i].rect.y + powerups[i].velY);

            // sair da tela
            if (powerups[i].rect.y > WINDOW_ALTURA || powerups[i].rect.x < -100 || powerups[i].rect.x > WINDOW_LARGURA + 100) {
                powerups[i].ativo = 0;
                continue;
            }

            // colisão com player
            if (SDL_HasIntersection(&powerups[i].rect, &player.rect)) {
                Mix_PlayChannel(-1, som_powerUp, 0);
                if (powerups[i].tipo == 0) {
                    player.saude += 3;
                    if (player.saude > PLAYER_MAX_SAUDE) player.saude = PLAYER_MAX_SAUDE;
                } else if (powerups[i].tipo == 1) {
                    dano_do_player *= 2;
                }
                powerups[i].ativo = 0;
            }
        }
    }
}

void update_inimigos() {
    // Velocidades
	float velX = inimigos_onda2 ? 2.0f : 1.4f;
	float velY = 10.0f;                         

	// Contador de passos horizontais
	static int passosHorizontais = 0;
	const int passosMax = 50;

	for (int i = 0; i < MAX_INIMIGOS; i++) {
    	if (!inimigos[i].ativo) continue;
    		inimigos[i].rect.x += (int)(velX * inimigo_dir);
	}

	bool tocouParede = false;
	for (int i = 0; i < MAX_INIMIGOS; i++) {
    	if (!inimigos[i].ativo) continue;

    	if (inimigos[i].rect.x < 10 ||
    	    inimigos[i].rect.x + inimigos[i].rect.w > WINDOW_LARGURA - 10) {
    	    tocouParede = true;
    	    break;
    	}
	}

	if (tocouParede) {
    	inimigo_dir *= -1;
    	passosHorizontais = passosMax;
	}

	passosHorizontais++;

	if (passosHorizontais >= passosMax) {
    	for (int i = 0; i < MAX_INIMIGOS; i++) {
        	if (!inimigos[i].ativo) continue;
        	inimigos[i].rect.y += (int)velY;
    	}
    	passosHorizontais = 0;
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
                    inimigos[j].saude -= dano_do_player;

                    if (inimigos[j].saude <= 0) {
                        // salvar a posição do ultimo inimigo morto
                        SDL_Rect ultimaPos = inimigos[j].rect;
                        inimigos[j].ativo = false;
                        pontuacao += 10;

                        // criar explosão
                        criar_explosao_em(ultimaPos);

                        // checar se todos os inimigos morreram
                        bool todosMortos = true;
                        for (int k = 0; k < MAX_INIMIGOS; k++) {
                            if (inimigos[k].ativo) {
                                todosMortos = false;
                                break;
                            }
                        }
                        
                        if (todosMortos && !powerups_spawned) {
                        Mix_PlayChannel(-1, som_explosao, 0);
                        Mix_PlayChannel(-1, som_explosao, 0);
                        Mix_PlayChannel(-1, som_explosao, 0);
                        // spawn de dois powerups na posição do ultimo inimigo
                        spawn_powerups_no(ultimaPos.x + ultimaPos.w/2, ultimaPos.y + ultimaPos.h/2);
                        powerups_spawned = true;

                        // iniciar contador para segunda onda (5 segundos)
                        tempo_para_segunda_onda = SDL_GetTicks() + 5000;
                    }
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
    render_texto(renderer, font_large, "VIAJANTES DO ESPACO",
                 WINDOW_LARGURA/2 - 520, WINDOW_ALTURA/6, white);

    const char* opcoes[] = { "INICIAR JOGO", "CREDITOS", "SAIR" };
    const int numOpcoes = 3;

    int larguraBotao = 350;
    int alturaBotao = 60;
    int espacamento = 30;

    int totalAltura = numOpcoes * alturaBotao + (numOpcoes - 1) * espacamento;
    int inicioY = (WINDOW_ALTURA / 2) - (totalAltura / 2) + 60;

    for (int i = 0; i < numOpcoes; i++) {
        int x = (WINDOW_LARGURA / 2) - (larguraBotao / 2);
        int y = inicioY + i * (alturaBotao + espacamento);

        SDL_Rect botao = { x, y, larguraBotao, alturaBotao };
        // Cor do botão
        SDL_SetRenderDrawColor(renderer, 50, 50, 150, 255);
        SDL_RenderFillRect(renderer, &botao);
        // Borda branca
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        for (int b = 0; b < 3; b++) {
    		SDL_Rect borda = { botao.x - b, botao.y - b, botao.w + 2*b, botao.h + 2*b };
    		SDL_RenderDrawRect(renderer, &borda);
	}

        int tw, th;
        TTF_SizeText(font_small, opcoes[i], &tw, &th);

        int textX = x + (larguraBotao - tw) / 2;
        int textY = y + (alturaBotao - th) / 2;

        render_texto(renderer, font_small, opcoes[i], textX, textY, white);
    }

    render_texto(renderer, font_smaller,
        "Pressione S para iniciar, C para creditos, Q para sair",
        WINDOW_LARGURA/2 - 350, WINDOW_ALTURA - 40, white);
}

void render_jogo(SDL_Renderer* renderer) {
    render_parallax(renderer);
    SDL_Color white = {255, 255, 255, 255};

    // 1. Nave do Jogador (textura)
    if (player_textura != NULL && (player.invencivel == 0 || player.invencivel % 10 < 5)) {
        SDL_RenderCopy(renderer, player_textura, NULL, &player.rect);
    }

    // Explosões (por cima do player e inimigos)
    for (int i = 0; i < MAX_EXPLOSOES; i++) {
        if (explosoes[i].ativo) {
            int frame = explosoes[i].frame;
            if (frame < EXPLOSAO_FRAMES && explosao_texturas[frame] != NULL) {
                SDL_RenderCopy(renderer, explosao_texturas[frame], NULL, &explosoes[i].rect);
            }
        }
    }
    
    // 2. Projéteis do Jogador (Amarelos ou Laranja se power-up de dano)
    if (dano_do_player > 1) {
        SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255); // laranja
    } else {
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255); // amarelo
    }
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
        if (inimigos[i].ativo) {
            SDL_Texture* tex = (inimigos_onda2 ? inimigo2_textura : inimigo_textura);
            if (tex != NULL) SDL_RenderCopy(renderer, tex, NULL, &inimigos[i].rect);
        }
    }

    // 5. Powerups
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (powerups[i].ativo) {
            SDL_Texture* tex = (powerups[i].tipo == 0 ? powerup_hp : powerup_damage);
            if (tex) SDL_RenderCopy(renderer, tex, NULL, &powerups[i].rect);
        }
    }

    // 6. Renderização do pontuacao (TTF)
    char pontuacao_str[50];
    sprintf(pontuacao_str, "PONTUACAO: %d", pontuacao);
    render_texto(renderer, font_smaller, pontuacao_str, 10, 10, white);

    // 7. Renderização da vida do jogador (barra de vida)
	int barraLarguraMax = 200;
	int barraAltura = 20;

	int barraX = WINDOW_LARGURA - barraLarguraMax - 20;
	int barraY = 20;

	// fundo vermelho
	SDL_Rect barraFundo = { barraX, barraY, barraLarguraMax, barraAltura };
	SDL_SetRenderDrawColor(renderer, 150, 0, 0, 255);
	SDL_RenderFillRect(renderer, &barraFundo);

	// parte verde proporcional à vida
	float proporcao = (float)player.saude / (float)PLAYER_MAX_SAUDE;
	int barraLargura = (int)(barraLarguraMax * proporcao);

	SDL_Rect barraVida = { barraX, barraY, barraLargura, barraAltura };
	SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
	SDL_RenderFillRect(renderer, &barraVida);

	// borda branca
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDrawRect(renderer, &barraFundo);
}

void render_game_over(SDL_Renderer* renderer) {
    render_parallax(renderer);
    SDL_Color red = {255, 0, 0, 255};
    SDL_Color white = {255, 255, 255, 255};
    
    render_texto(renderer, font_large, "GAME OVER", WINDOW_LARGURA/2 - 250, WINDOW_ALTURA/2 - 80, red);

    char pontuacao_str[50];
    sprintf(pontuacao_str, "PONTUACAO FINAL: %d", pontuacao);
    int text_w, text_h;
    TTF_SizeText(font_small, pontuacao_str, &text_w, &text_h);
    render_texto(renderer, font_small, pontuacao_str, WINDOW_LARGURA/2 - text_w/2, WINDOW_ALTURA/2 + 20, white);

    render_texto(renderer, font_smaller, "Pressione ESC para voltar ao Menu", WINDOW_LARGURA/2 - 200, WINDOW_ALTURA - 50, white);
}

void render_creditos(SDL_Renderer* renderer) {
    render_parallax(renderer);
    SDL_Color white = {255, 255, 255, 255};
    
    render_texto(renderer, font_medium, "CREDITOS", WINDOW_LARGURA/2 - 100, WINDOW_ALTURA/4 - 50, white);
    
    render_texto(renderer, font_small, "Desenvolvedor: Clarissa Berlim", WINDOW_LARGURA/2 - 250, WINDOW_ALTURA/2 - 50, white);
    render_texto(renderer, font_small, "Ano: 2025", WINDOW_LARGURA/2 - 250, WINDOW_ALTURA/2 + 20, white);
    render_texto(renderer, font_small, "Agradecimentos: ....", WINDOW_LARGURA/2 - 250, WINDOW_ALTURA/2 + 90, white);

    render_texto(renderer, font_smaller, "Pressione ESC para voltar ao Menu", WINDOW_LARGURA/2 - 250, WINDOW_ALTURA - 50, white);
}


// --- FUNÇÃO LIMPEZA ---
void limpar() {
    // Texturas
    SDL_DestroyTexture(player_textura);
    SDL_DestroyTexture(inimigo_textura);
    SDL_DestroyTexture(inimigo2_textura);
    for (int i = 0; i < TEXTURAS_METEOROS; i++) {
        SDL_DestroyTexture(meteoro_textura[i]);
    }
    for (int i = 0; i < EXPLOSAO_FRAMES; i++) {
        if (explosao_texturas[i]) SDL_DestroyTexture(explosao_texturas[i]);
    }
    SDL_DestroyTexture(powerup_hp);
    SDL_DestroyTexture(powerup_damage);
    IMG_Quit();
    
    // TTF
    TTF_CloseFont(font_smaller);
    TTF_CloseFont(font_small);
    TTF_CloseFont(font_medium);
    TTF_CloseFont(font_large);
    TTF_Quit();

    // Mixer
    Mix_FreeMusic(musica_geral);
    Mix_FreeChunk(som_tiros);
    Mix_FreeChunk(som_gameover);
    Mix_FreeChunk(som_explosao);
    Mix_FreeChunk(som_powerUp);
    Mix_CloseAudio();
    
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
            
            case SDL_SCANCODE_ESCAPE:
            	estado_atual = SAIR;
            	rodando = false;
            	break;
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
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
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

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("Erro ao iniciar SDL_mixer: %s\n", Mix_GetError());
    }

    font_smaller = TTF_OpenFont(FONT_PATH, 14);
    font_small = TTF_OpenFont(FONT_PATH, 18);
    font_medium = TTF_OpenFont(FONT_PATH, 30);
    font_large = TTF_OpenFont(FONT_PATH, 60);

    if (!font_smaller || !font_small || !font_medium || !font_large) {
        printf("Erro ao carregar fonte (%s): %s\n", FONT_PATH, TTF_GetError());
    }
    
    // criar janela para ficar em tela cheia
    SDL_Window* window = SDL_CreateWindow("VIAJANTES DO ESPACO", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    						WINDOW_LARGURA, WINDOW_ALTURA, SDL_WINDOW_SHOWN | SDL_WINDOW_MAXIMIZED);

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

    // Carregar sons
    musica_geral = Mix_LoadMUS("assets/soundtrack.mp3");
    som_tiros = Mix_LoadWAV("assets/sound_playerTiros.wav");
    som_gameover = Mix_LoadWAV("assets/sound_gameOver.wav");
    som_explosao = Mix_LoadWAV("assets/sound_explosao.wav");
    som_powerUp = Mix_LoadWAV("assets/sound_powerUp.wav");

    if (!musica_geral || !som_tiros || !som_gameover || !som_explosao || !som_powerUp) {
        printf("Erro carregando audio: %s\n", Mix_GetError());
    }

    resetar_jogo(); 

    Mix_PlayMusic(musica_geral, -1); // -1 = loop infinito para o soundtrack ficar tocando sempre
    
    SDL_Event e;
    
    while (rodando) {
        if (SDL_WaitEventTimeout(&e, DELAY_FRAME)) {
            if (e.type == SDL_MOUSEMOTION ||
    			e.type == SDL_MOUSEBUTTONDOWN ||
			    e.type == SDL_MOUSEBUTTONUP ||
			    e.type == SDL_MOUSEWHEEL) {
			    // descarta o evento
			}

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
                    if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
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
            update_explosoes();
            update_powerups();

            // checar se é hora de spawnar a segunda onda (5s após todos mortos)
            if (!segunda_onda_spawnada && powerups_spawned && tempo_para_segunda_onda > 0) {
                if (SDL_GetTicks() >= tempo_para_segunda_onda) {
                    iniciar_inimigos_onda2();
                    segunda_onda_spawnada = true;
                }
            }
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

#ifndef JOGO_H
#define JOGO_H
#include <stdbool.h>
#include "config.h"
#define MAXIMO_CELULAS (LARGURA_TABULEIRO * ALTURA_TABULEIRO)
typedef struct { int x; int y; } Celula;
typedef enum { CIMA, BAIXO, ESQUERDA, DIREITA } Direcao;
typedef enum { JOGANDO, PERDEU, VENCEU } SituacaoJogo;
typedef struct {
    Celula cobra[MAXIMO_CELULAS];
    int comprimento;
    Celula alimento;
    bool obstaculos[ALTURA_TABULEIRO][LARGURA_TABULEIRO];
    Direcao direcao;
    SituacaoJogo situacao;
    int pontuacao;
} Jogo;
void iniciar_jogo(Jogo *jogo, bool obstaculos);
void avancar_jogo(Jogo *jogo, Direcao direcao);
int intervalo_jogo_ms(const Jogo *jogo);
#endif

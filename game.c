/* Regras independentes do servidor e do navegador. */
#include "game.h"
#include <stdlib.h>
#include <string.h>

_Static_assert(LARGURA_TABULEIRO >= 4 && ALTURA_TABULEIRO >= 3, "Tabuleiro pequeno demais");
_Static_assert(COMPRIMENTO_INICIAL >= 2 && COMPRIMENTO_INICIAL <= LARGURA_TABULEIRO / 2 + 1, "Comprimento invalido");
_Static_assert(QUANTIDADE_OBSTACULOS >= 0 && QUANTIDADE_OBSTACULOS <= LARGURA_TABULEIRO * (ALTURA_TABULEIRO - 1), "Obstaculos invalidos");
_Static_assert(INTERVALO_MINIMO_MS > 0 && INTERVALO_INICIAL_MS >= INTERVALO_MINIMO_MS && ACELERACAO_MS >= 0, "Velocidade invalida");

static bool mesma_celula(Celula primeira, Celula segunda) {
    return primeira.x == segunda.x && primeira.y == segunda.y;
}

static void posicionar_alimento(Jogo *jogo) {
    bool ocupadas[ALTURA_TABULEIRO][LARGURA_TABULEIRO];
    memcpy(ocupadas, jogo->obstaculos, sizeof(ocupadas));
    for (int indice = 0; indice < jogo->comprimento; indice++) {
        ocupadas[jogo->cobra[indice].y][jogo->cobra[indice].x] = true;
    }
    Celula celulas_livres[MAXIMO_CELULAS];
    int quantidade = 0;
    for (int y = 0; y < ALTURA_TABULEIRO; y++) {
        for (int x = 0; x < LARGURA_TABULEIRO; x++) {
            if (!ocupadas[y][x]) celulas_livres[quantidade++] = (Celula){x, y};
        }
    }
    jogo->alimento = quantidade ? celulas_livres[rand() % quantidade] : (Celula){-1, -1};
    if (!quantidade) jogo->situacao = VENCEU;
}

void iniciar_jogo(Jogo *jogo, bool obstaculos) {
    memset(jogo, 0, sizeof(*jogo));
    jogo->comprimento = COMPRIMENTO_INICIAL;
    jogo->direcao = DIREITA;
    jogo->situacao = JOGANDO;
    for (int indice = 0; indice < jogo->comprimento; indice++) {
        jogo->cobra[indice] = (Celula){LARGURA_TABULEIRO / 2 - indice, ALTURA_TABULEIRO / 2};
    }
    if (obstaculos) {
        Celula candidatas[MAXIMO_CELULAS];
        int quantidade = 0;
        for (int y = 0; y < ALTURA_TABULEIRO; y++) {
            for (int x = 0; x < LARGURA_TABULEIRO; x++) {
                /* Mantém a linha de partida livre. */
                if (y != ALTURA_TABULEIRO / 2) candidatas[quantidade++] = (Celula){x, y};
            }
        }
        for (int indice = 0; indice < QUANTIDADE_OBSTACULOS; indice++) {
            int escolhida = rand() % quantidade;
            Celula celula = candidatas[escolhida];
            jogo->obstaculos[celula.y][celula.x] = true;
            candidatas[escolhida] = candidatas[--quantidade];
        }
    }
    posicionar_alimento(jogo);
}

void avancar_jogo(Jogo *jogo, Direcao direcao) {
    static const int deslocamento_x[] = {0, 0, -1, 1};
    static const int deslocamento_y[] = {-1, 1, 0, 0};
    if (jogo->situacao != JOGANDO) return;
    if (direcao >= CIMA && direcao <= DIREITA &&
        !(deslocamento_x[direcao] == -deslocamento_x[jogo->direcao] && deslocamento_y[direcao] == -deslocamento_y[jogo->direcao])) {
        jogo->direcao = direcao;
    }
    Celula cabeca = {jogo->cobra[0].x + deslocamento_x[jogo->direcao], jogo->cobra[0].y + deslocamento_y[jogo->direcao]};
    bool crescendo = mesma_celula(cabeca, jogo->alimento);
    if (cabeca.x < 0 || cabeca.x >= LARGURA_TABULEIRO || cabeca.y < 0 || cabeca.y >= ALTURA_TABULEIRO ||
        jogo->obstaculos[cabeca.y][cabeca.x]) {
        jogo->situacao = PERDEU;
        return;
    }
    /* A cauda libera sua célula neste turno, exceto quando a cobra come. */
    int comprimento_corpo = jogo->comprimento - (crescendo ? 0 : 1);
    for (int indice = 0; indice < comprimento_corpo; indice++) {
        if (mesma_celula(cabeca, jogo->cobra[indice])) {
            jogo->situacao = PERDEU;
            return;
        }
    }
    memmove(jogo->cobra + 1, jogo->cobra, (size_t)comprimento_corpo * sizeof(Celula));
    jogo->cobra[0] = cabeca;
    if (crescendo) {
        jogo->comprimento++;
        jogo->pontuacao += PONTOS_POR_ALIMENTO;
        posicionar_alimento(jogo);
    }
}

int intervalo_jogo_ms(const Jogo *jogo) {
    int intervalo = INTERVALO_INICIAL_MS - (jogo->comprimento - COMPRIMENTO_INICIAL) * ACELERACAO_MS;
    return intervalo < INTERVALO_MINIMO_MS ? INTERVALO_MINIMO_MS : intervalo;
}

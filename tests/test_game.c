#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "game.h"

int main(void) {
    srand(1);
    Jogo jogo;
    iniciar_jogo(&jogo, false);
    Celula cabeca = jogo.cobra[0];
    jogo.alimento = (Celula){0, 0};
    avancar_jogo(&jogo, ESQUERDA);
    assert(jogo.direcao == DIREITA && jogo.cobra[0].x == cabeca.x + 1);
    assert(jogo.comprimento == COMPRIMENTO_INICIAL);

    jogo.alimento = (Celula){jogo.cobra[0].x + 1, jogo.cobra[0].y};
    avancar_jogo(&jogo, DIREITA);
    assert(jogo.comprimento == COMPRIMENTO_INICIAL + 1 && jogo.pontuacao == PONTOS_POR_ALIMENTO);
    assert(intervalo_jogo_ms(&jogo) <= INTERVALO_INICIAL_MS);
    for (int indice = 0; indice < jogo.comprimento; indice++) {
        assert(jogo.alimento.x != jogo.cobra[indice].x || jogo.alimento.y != jogo.cobra[indice].y);
    }
    while (jogo.situacao == JOGANDO) avancar_jogo(&jogo, DIREITA);
    assert(jogo.situacao == PERDEU);
    cabeca = jogo.cobra[0];
    avancar_jogo(&jogo, CIMA);
    assert(jogo.cobra[0].x == cabeca.x && jogo.cobra[0].y == cabeca.y);

    iniciar_jogo(&jogo, true);
    int obstaculos = 0;
    for (int y = 0; y < ALTURA_TABULEIRO; y++) {
        for (int x = 0; x < LARGURA_TABULEIRO; x++) obstaculos += jogo.obstaculos[y][x];
    }
    assert(obstaculos == QUANTIDADE_OBSTACULOS);
    assert(!jogo.obstaculos[jogo.alimento.y][jogo.alimento.x]);
    jogo.obstaculos[jogo.cobra[0].y][jogo.cobra[0].x + 1] = true;
    avancar_jogo(&jogo, DIREITA);
    assert(jogo.situacao == PERDEU);

    iniciar_jogo(&jogo, false);
    jogo.comprimento = 4;
    jogo.cobra[0] = (Celula){2, 2};
    jogo.cobra[1] = (Celula){2, 3};
    jogo.cobra[2] = (Celula){1, 3};
    jogo.cobra[3] = (Celula){1, 2};
    jogo.alimento = (Celula){0, 0};
    jogo.direcao = CIMA;
    avancar_jogo(&jogo, ESQUERDA); /* Pode entrar na célula liberada pela cauda. */
    assert(jogo.situacao == JOGANDO && jogo.cobra[0].x == 1);
    jogo.direcao = CIMA;
    avancar_jogo(&jogo, DIREITA); /* Colisão com o corpo. */
    assert(jogo.situacao == PERDEU);

    iniciar_jogo(&jogo, false);
    jogo.comprimento = 2;
    jogo.cobra[0] = (Celula){1, 0};
    jogo.cobra[1] = (Celula){0, 0};
    jogo.alimento = (Celula){2, 0};
    for (int y = 0; y < ALTURA_TABULEIRO; y++) {
        for (int x = 0; x < LARGURA_TABULEIRO; x++) jogo.obstaculos[y][x] = true;
    }
    jogo.obstaculos[0][0] = jogo.obstaculos[0][1] = jogo.obstaculos[0][2] = false;
    avancar_jogo(&jogo, DIREITA);
    assert(jogo.situacao == VENCEU && jogo.alimento.x == -1);
    puts("Todos os testes de regras passaram.");
    return 0;
}

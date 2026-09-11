/* Servidor HTTP para uso local. Execute na pasta raiz do projeto. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include "game.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Socket;
#define FECHAR_SOCKET closesocket
#else
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
typedef int Socket;
#define INVALID_SOCKET (-1)
#define FECHAR_SOCKET close
#endif

typedef struct {
    unsigned int id;
    time_t ultimo_acesso;
    Jogo jogo;
} Sessao;

static Sessao sessoes[MAXIMO_PARTIDAS];
static unsigned int proximo_id = 1;

static void enviar_tudo(Socket conexao, const char *dados, size_t comprimento) {
    while (comprimento) {
        int enviados = send(conexao, dados, (int)comprimento, 0);
        if (enviados <= 0) return;
        dados += enviados;
        comprimento -= (size_t)enviados;
    }
}

static void responder(Socket conexao, int situacao, const char *tipo, const char *corpo) {
    char cabecalho[512];
    snprintf(cabecalho, sizeof(cabecalho),
             "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
             "Connection: close\r\nCache-Control: no-store\r\n"
             "X-Content-Type-Options: nosniff\r\n\r\n",
             situacao, situacao == 200 ? "OK" : "Error", tipo, strlen(corpo));
    enviar_tudo(conexao, cabecalho, strlen(cabecalho));
    enviar_tudo(conexao, corpo, strlen(corpo));
}

static void responder_erro(Socket conexao, int situacao, const char *mensagem) {
    char json[256];
    snprintf(json, sizeof(json), "{\"error\":\"%s\"}", mensagem);
    responder(conexao, situacao, "application/json; charset=utf-8", json);
}

static void acrescentar(char *buffer, size_t capacidade, size_t *utilizados, const char *formato, ...) {
    va_list argumentos;
    va_start(argumentos, formato);
    int escritos = vsnprintf(buffer + *utilizados, capacidade - *utilizados, formato, argumentos);
    va_end(argumentos);
    if (escritos < 0 || (size_t)escritos >= capacidade - *utilizados) {
        fprintf(stderr, "Buffer JSON insuficiente\n");
        exit(EXIT_FAILURE);
    }
    *utilizados += (size_t)escritos;
}

static void enviar_estado(Socket conexao, const Sessao *sessao) {
    char json[MAXIMO_CELULAS * 64 + 2048];
    size_t utilizados = 0;
    const Jogo *jogo = &sessao->jogo;
    const char *direcoes[] = {"up", "down", "left", "right"};
    const char *situacoes[] = {"playing", "lost", "won"};
    acrescentar(json, sizeof(json), &utilizados, "{\"id\":%u,\"title\":\"", sessao->id);
    for (const unsigned char *caractere = (const unsigned char *)TITULO_JOGO; *caractere; caractere++) {
        if (*caractere == '"' || *caractere == '\\' || *caractere < 32) {
            acrescentar(json, sizeof(json), &utilizados, "\\u%04x", *caractere);
        } else {
            acrescentar(json, sizeof(json), &utilizados, "%c", *caractere);
        }
    }
    acrescentar(json, sizeof(json), &utilizados,
           "\",\"width\":%d,\"height\":%d,\"score\":%d,\"step_ms\":%d,"
           "\"level\":%d,\"direction\":\"%s\",\"status\":\"%s\",\"snake\":[",
           LARGURA_TABULEIRO, ALTURA_TABULEIRO, jogo->pontuacao, intervalo_jogo_ms(jogo),
           1 + (jogo->comprimento - COMPRIMENTO_INICIAL) / 5,
           direcoes[jogo->direcao], situacoes[jogo->situacao]);
    for (int indice = 0; indice < jogo->comprimento; indice++) {
        acrescentar(json, sizeof(json), &utilizados, "%s[%d,%d]", indice ? "," : "", jogo->cobra[indice].x, jogo->cobra[indice].y);
    }
    acrescentar(json, sizeof(json), &utilizados, "],\"food\":");
    if (jogo->alimento.x < 0) acrescentar(json, sizeof(json), &utilizados, "null");
    else acrescentar(json, sizeof(json), &utilizados, "[%d,%d]", jogo->alimento.x, jogo->alimento.y);
    acrescentar(json, sizeof(json), &utilizados, ",\"obstacles\":[");
    int quantidade = 0;
    for (int y = 0; y < ALTURA_TABULEIRO; y++) {
        for (int x = 0; x < LARGURA_TABULEIRO; x++) {
            if (jogo->obstaculos[y][x]) {
                acrescentar(json, sizeof(json), &utilizados, "%s[%d,%d]", quantidade++ ? "," : "", x, y);
            }
        }
    }
    acrescentar(json, sizeof(json), &utilizados, "]}");
    responder(conexao, 200, "application/json; charset=utf-8", json);
}

static void servir_arquivo(Socket conexao, const char *caminho, const char *tipo) {
    FILE *arquivo = fopen(caminho, "rb");
    if (!arquivo) {
        responder_erro(conexao, 500, "Arquivo da interface indisponivel.");
        return;
    }
    char dados[65536];
    size_t comprimento = fread(dados, 1, sizeof(dados) - 1, arquivo);
    bool falhou = ferror(arquivo) || !feof(arquivo);
    fclose(arquivo);
    if (falhou) {
        responder_erro(conexao, 500, "Arquivo da interface grande demais.");
        return;
    }
    dados[comprimento] = '\0';
    responder(conexao, 200, tipo, dados);
}

static bool cabecalho_corresponde(const char *linha, const char *nome) {
    while (*nome) {
        if (tolower((unsigned char)*linha++) != tolower((unsigned char)*nome++)) return false;
    }
    return true;
}

static void atender_requisicao(Socket conexao) {
    char requisicao[8192];
    size_t utilizados = 0;
    char *fim = NULL;
    while (utilizados < sizeof(requisicao) - 1) {
        int recebidos = recv(conexao, requisicao + utilizados, (int)(sizeof(requisicao) - 1 - utilizados), 0);
        if (recebidos <= 0) return;
        utilizados += (size_t)recebidos;
        requisicao[utilizados] = '\0';
        fim = strstr(requisicao, "\r\n\r\n");
        if (fim) break;
    }
    if (!fim) {
        responder_erro(conexao, 400, "Cabecalho muito grande.");
        return;
    }
    char metodo[8], caminho[128];
    if (sscanf(requisicao, "%7s %127s", metodo, caminho) != 2) {
        responder_erro(conexao, 400, "Pedido invalido.");
        return;
    }
    if (!strcmp(metodo, "GET")) {
        if (!strcmp(caminho, "/")) servir_arquivo(conexao, "static/index.html", "text/html; charset=utf-8");
        else if (!strcmp(caminho, "/app.js")) servir_arquivo(conexao, "static/app.js", "text/javascript; charset=utf-8");
        else if (!strcmp(caminho, "/style.css")) servir_arquivo(conexao, "static/style.css", "text/css; charset=utf-8");
        else if (!strcmp(caminho, "/health")) responder(conexao, 200, "application/json", "{\"status\":\"ok\"}");
        else responder_erro(conexao, 404, "Pagina nao encontrada.");
        return;
    }
    if (strcmp(metodo, "POST") || (strcmp(caminho, "/api/new") && strcmp(caminho, "/api/step"))) {
        responder_erro(conexao, 404, "Rota nao encontrada.");
        return;
    }
    long comprimento = -1;
    for (char *linha = strstr(requisicao, "\r\n") + 2; linha < fim; ) {
        if (cabecalho_corresponde(linha, "Content-Length:")) {
            char *restante;
            long valor_lido = strtol(linha + 15, &restante, 10);
            while (*restante == ' ' || *restante == '\t') restante++;
            if (comprimento != -1 || restante == linha + 15 || strncmp(restante, "\r\n", 2)) {
                responder_erro(conexao, 400, "Tamanho invalido.");
                return;
            }
            comprimento = valor_lido;
        }
        if (cabecalho_corresponde(linha, "Transfer-Encoding:")) {
            responder_erro(conexao, 400, "Formato nao suportado.");
            return;
        }
        linha = strstr(linha, "\r\n") + 2;
    }
    size_t deslocamento = (size_t)(fim + 4 - requisicao);
    if (comprimento < 1 || comprimento > 128 || deslocamento + (size_t)comprimento >= sizeof(requisicao)) {
        responder_erro(conexao, 400, "Tamanho invalido.");
        return;
    }
    while (utilizados < deslocamento + (size_t)comprimento) {
        int recebidos = recv(conexao, requisicao + utilizados, (int)(deslocamento + (size_t)comprimento - utilizados), 0);
        if (recebidos <= 0) return;
        utilizados += (size_t)recebidos;
    }
    requisicao[deslocamento + (size_t)comprimento] = '\0';
    /* Protocolo: identificador numérico, direção e opção de obstáculos. */
    unsigned int id, obstaculos;
    char direcao[6], excedente;
    if (sscanf(requisicao + deslocamento, "%u %5s %u %c", &id, direcao, &obstaculos, &excedente) != 3 || obstaculos > 1) {
        responder_erro(conexao, 400, "Pedido invalido.");
        return;
    }
    Direcao movimento;
    if (!strcmp(direcao, "up")) movimento = CIMA;
    else if (!strcmp(direcao, "down")) movimento = BAIXO;
    else if (!strcmp(direcao, "left")) movimento = ESQUERDA;
    else if (!strcmp(direcao, "right")) movimento = DIREITA;
    else {
        responder_erro(conexao, 400, "Direcao invalida.");
        return;
    }
    time_t agora = time(NULL);
    Sessao *existente = NULL, *disponivel = NULL;
    for (int indice = 0; indice < MAXIMO_PARTIDAS; indice++) {
        if (sessoes[indice].id && difftime(agora, sessoes[indice].ultimo_acesso) > VALIDADE_PARTIDA_SEGUNDOS) sessoes[indice].id = 0;
        if (id && sessoes[indice].id == id) existente = &sessoes[indice];
        if (!sessoes[indice].id && !disponivel) disponivel = &sessoes[indice];
    }
    if (!strcmp(caminho, "/api/new")) {
        Sessao *sessao = existente ? existente : disponivel;
        if (!sessao || proximo_id == UINT_MAX) {
            responder_erro(conexao, 503, "Servidor cheio. Tente mais tarde.");
            return;
        }
        sessao->id = proximo_id++;
        sessao->ultimo_acesso = agora;
        iniciar_jogo(&sessao->jogo, obstaculos != 0);
        enviar_estado(conexao, sessao);
    } else if (existente) {
        existente->ultimo_acesso = agora;
        avancar_jogo(&existente->jogo, movimento);
        enviar_estado(conexao, existente);
    } else {
        responder_erro(conexao, 404, "Partida expirada. Reinicie o jogo.");
    }
}

int main(void) {
#ifdef _WIN32
    WSADATA dados;
    if (WSAStartup(MAKEWORD(2, 2), &dados)) return EXIT_FAILURE;
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    srand((unsigned int)time(NULL));
    Socket socket_servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_servidor == INVALID_SOCKET) return EXIT_FAILURE;
    int reutilizar = 1;
    setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, (const char *)&reutilizar, sizeof(reutilizar));
    struct sockaddr_in endereco = {0};
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = htonl(INADDR_ANY);
    endereco.sin_port = htons(PORTA_SERVIDOR);
    if (bind(socket_servidor, (struct sockaddr *)&endereco, sizeof(endereco)) || listen(socket_servidor, 16)) {
        fprintf(stderr, "Nao foi possivel abrir a porta %d.\n", PORTA_SERVIDOR);
        FECHAR_SOCKET(socket_servidor);
        return EXIT_FAILURE;
    }
    printf("Cobrinha em http://localhost:%d\n", PORTA_SERVIDOR);
    fflush(stdout);
    for (;;) {
        Socket cliente = accept(socket_servidor, NULL, NULL);
        if (cliente == INVALID_SOCKET) continue;
#ifdef _WIN32
        DWORD tempo_limite = 2000;
#else
        struct timeval tempo_limite = {2, 0};
#endif
        setsockopt(cliente, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tempo_limite, sizeof(tempo_limite));
        setsockopt(cliente, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tempo_limite, sizeof(tempo_limite));
        atender_requisicao(cliente);
        FECHAR_SOCKET(cliente);
    }
}

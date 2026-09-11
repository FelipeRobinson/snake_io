# Cobrinha Neon em C

Jogo com visual neon, velocidade progressiva, obstáculos opcionais, pausa,
controles por teclado/toque e recorde salvo no navegador.

O servidor e todas as regras são escritos em **C11**, sem dependências externas.
HTML, CSS e JavaScript desenham a interface no navegador. Não há código Python.
O código usa indentação de quatro espaços e comentários em português.
Variáveis, funções próprias, tipos e constantes usam nomes em português, sem
acentos: `pontuacao`, `iniciar_jogo`, `Celula` e `COMPRIMENTO_INICIAL`.
Os nomes das bibliotecas e os campos do protocolo HTTP/JSON foram preservados.

## Rodar com Docker

Com o Docker Desktop iniciado, usando containers Linux, execute nesta pasta:

```sh
docker compose up --build -d
```

Abra [o jogo](http://localhost:8000) e clique em **Iniciar**.
O build compila o C e executa os testes antes de gerar a imagem final.

```sh
docker compose logs -f
docker compose down
```

Após editar os arquivos, execute novamente `docker compose up --build -d`.
Se a porta estiver ocupada, altere o lado esquerdo do mapeamento em
`compose.yaml` para `127.0.0.1:8080:8000` e acesse a porta 8080.

## Controles

| Ação | Controle |
| --- | --- |
| Mover | Setas, WASD ou botões na tela |
| Pausar / continuar | Espaço ou botão Pausar |
| Reiniciar | R ou botão Reiniciar |
| Ativar obstáculos | Marque a opção antes de iniciar outra partida |

Coma os pontos rosas e evite paredes, obstáculos e o próprio corpo.
A cada alimento a cobra cresce e acelera, até o limite configurado.
O jogo pausa ao sair da aba. A partida fica na memória do servidor;
reiniciar o container encerra as partidas. O recorde permanece no navegador.

## Personalização

| Arquivo | O que alterar |
| --- | --- |
| `config.h` | Título, tamanho do tabuleiro, velocidade, pontos e obstáculos |
| `game.c` / `game.h` | Movimento, crescimento, colisões e estado do jogo |
| `server.c` | Servidor HTTP e comunicação com o navegador |
| `static/style.css` | Cores nas variáveis `:root`, fontes e disposição |
| `static/index.html` | Textos e botões |
| `static/app.js` | Desenho do tabuleiro e controles |
| `tests/test_game.c` | Testes das regras |

Por exemplo: use `INTERVALO_INICIAL_MS 200` para começar mais devagar,
`QUANTIDADE_OBSTACULOS 20` para adicionar obstáculos e altere `--cobra` no CSS
para mudar a cor da cobra. O comprimento inicial precisa caber na metade
esquerda do tabuleiro; o compilador verifica os limites básicos.

## Compilar sem Docker

Linux, com GCC instalado:

```sh
mkdir -p build
gcc -std=c11 -O2 -Wall -Wextra -Werror -pedantic game.c server.c -o build/snake
./build/snake
```

Windows / PowerShell, com GCC do MinGW no PATH:

```powershell
New-Item -ItemType Directory -Force build
gcc -std=c11 -O2 -Wall -Wextra -Werror -pedantic game.c server.c -o build/snake.exe -lws2_32
.\build\snake.exe
```

Execute sempre na raiz do projeto para encontrar a pasta `static`.
Use Ctrl+C para parar. O servidor é simples e destinado a uso local;
o Docker Compose publica a porta apenas em localhost.

## Testes

```sh
gcc -std=c11 -Wall -Wextra -Werror -pedantic -I. game.c tests/test_game.c -o build/test_game
./build/test_game
```

No Windows, acrescente `.exe` ao nome do executável.

// O navegador desenha o tabuleiro; as regras e colisões rodam em C.
const tela = document.querySelector('#game');
const contexto = tela.getContext('2d');
const sobreposicao = document.querySelector('#overlay');
const botaoPausar = document.querySelector('#pause');
const botaoIniciar = document.querySelector('#start');
const cores = getComputedStyle(document.documentElement);
const cor = nome => cores.getPropertyValue(nome).trim();
let estado = null;
let idPartida = null;
let pausado = false;
let temporizador = null;
let geracao = 0;
let requisicaoEmAndamento = null;
let direcoes = [];
let recorde = 0;
try { recorde = Number(localStorage.getItem('snake-best')) || 0; } catch (_) {}
document.querySelector('#best').textContent = recorde;

async function api(caminho, dadosPedido) {
    const resposta = await fetch(caminho, {
        method: 'POST',
        headers: { 'Content-Type': 'text/plain' },
        body: `${dadosPedido.id || 0} ${dadosPedido.direction || 'right'} ${dadosPedido.obstacles ? 1 : 0}`,
    });
    const dados = await resposta.json();
    if (!resposta.ok) throw new Error(dados.error || 'Falha ao acessar o jogo.');
    return dados;
}

function mostrarMensagem(titulo, dica) {
    document.querySelector('#message').textContent = titulo;
    document.querySelector('#hint').textContent = dica;
    sobreposicao.hidden = false;
}

function desenhar() {
    const tamanho = 32;
    tela.width = estado.width * tamanho;
    tela.height = estado.height * tamanho;
    contexto.strokeStyle = cor('--grade');
    for (let x = 0; x <= tela.width; x += tamanho) {
        contexto.beginPath(); contexto.moveTo(x, 0); contexto.lineTo(x, tela.height); contexto.stroke();
    }
    for (let y = 0; y <= tela.height; y += tamanho) {
        contexto.beginPath(); contexto.moveTo(0, y); contexto.lineTo(tela.width, y); contexto.stroke();
    }
    function desenharBloco([x, y], preenchimento, margemInterna = 3) {
        contexto.fillStyle = preenchimento;
        contexto.beginPath();
        contexto.roundRect(x * tamanho + margemInterna, y * tamanho + margemInterna, tamanho - margemInterna * 2, tamanho - margemInterna * 2, 7);
        contexto.fill();
    }
    estado.obstacles.forEach(celula => desenharBloco(celula, cor('--obstaculo')));
    estado.snake.forEach((celula, indice) => desenharBloco(celula, cor(indice ? '--cobra' : '--cabeca')));
    if (estado.food) {
        contexto.fillStyle = cor('--alimento');
        contexto.shadowColor = cor('--alimento');
        contexto.shadowBlur = 18;
        contexto.beginPath();
        contexto.arc(estado.food[0] * tamanho + tamanho / 2, estado.food[1] * tamanho + tamanho / 2, 9, 0, Math.PI * 2);
        contexto.fill();
        contexto.shadowBlur = 0;
    }
    document.querySelector('#score').textContent = estado.score;
    document.querySelector('#level').textContent = estado.level;
    if (estado.score > recorde) {
        recorde = estado.score;
        try { localStorage.setItem('snake-best', recorde); } catch (_) {}
    }
    document.querySelector('#best').textContent = recorde;
}

function agendar(geracaoAtual) {
    clearTimeout(temporizador);
    if (!pausado && estado?.status === 'playing') {
        temporizador = setTimeout(() => avancar(geracaoAtual), estado.step_ms);
    }
}

async function avancar(geracaoAtual) {
    if (geracaoAtual !== geracao || requisicaoEmAndamento === geracaoAtual) return;
    requisicaoEmAndamento = geracaoAtual;
    try {
        const proximoEstado = await api('/api/step', { id: idPartida, direction: direcoes.shift() || estado.direction });
        if (geracaoAtual !== geracao) return;
        requisicaoEmAndamento = null;
        estado = proximoEstado;
        desenhar();
        if (estado.status !== 'playing') {
            botaoPausar.disabled = true;
            mostrarMensagem(estado.status === 'won' ? 'Você venceu!' : 'Fim de jogo!', `Você fez ${estado.score} pontos. Clique em Reiniciar para jogar de novo.`);
        } else {
            agendar(geracaoAtual);
        }
    } catch (erro) {
        if (geracaoAtual !== geracao) return;
        requisicaoEmAndamento = null;
        pausado = true;
        botaoPausar.disabled = true;
        document.querySelector('#error').textContent = erro.message;
        mostrarMensagem('Conexão interrompida', 'Verifique o servidor e clique em Reiniciar.');
    }
}

async function iniciar() {
    const geracaoAtual = ++geracao;
    clearTimeout(temporizador);
    botaoIniciar.disabled = true;
    botaoPausar.disabled = true;
    document.querySelector('#error').textContent = '';
    try {
        estado = await api('/api/new', { id: idPartida, obstacles: document.querySelector('#obstacles').checked });
        idPartida = estado.id;
        direcoes = [];
        pausado = false;
        sobreposicao.hidden = true;
        botaoPausar.textContent = 'Pausar';
        botaoPausar.disabled = false;
        botaoIniciar.textContent = 'Reiniciar';
        document.querySelector('#title').textContent = estado.title;
        document.title = estado.title;
        desenhar();
        agendar(geracaoAtual);
    } catch (erro) {
        document.querySelector('#error').textContent = erro.message;
    } finally {
        botaoIniciar.disabled = false;
    }
}

function alternarPausa() {
    if (botaoPausar.disabled || !estado || estado.status !== 'playing') return;
    pausado = !pausado;
    botaoPausar.textContent = pausado ? 'Continuar' : 'Pausar';
    if (pausado) {
        clearTimeout(temporizador);
        mostrarMensagem('Pausa', 'Respire. Pressione Espaço ou Continuar para voltar.');
    } else {
        sobreposicao.hidden = true;
        agendar(geracao);
    }
}

function mudarDirecao(direcao) {
    if (pausado || estado?.status !== 'playing') return;
    const opostas = { up: 'down', down: 'up', left: 'right', right: 'left' };
    const anterior = direcoes.at(-1) || estado.direction;
    if (direcao !== anterior && direcao !== opostas[anterior] && direcoes.length < 2) {
        direcoes.push(direcao);
    }
}

botaoIniciar.addEventListener('click', iniciar);
botaoPausar.addEventListener('click', alternarPausa);
document.querySelectorAll('[data-dir]').forEach(botao => {
    botao.addEventListener('click', () => mudarDirecao(botao.dataset.dir));
});
document.addEventListener('keydown', evento => {
    if (evento.target.matches('input')) return;
    const teclas = { ArrowUp: 'up', w: 'up', ArrowDown: 'down', s: 'down', ArrowLeft: 'left', a: 'left', ArrowRight: 'right', d: 'right' };
    const tecla = evento.key.length === 1 ? evento.key.toLowerCase() : evento.key;
    if (teclas[tecla]) { evento.preventDefault(); mudarDirecao(teclas[tecla]); }
    if (tecla === ' ' && !evento.repeat) { evento.preventDefault(); alternarPausa(); }
    if (tecla === 'r' && !evento.repeat && !botaoIniciar.disabled) iniciar();
});
document.addEventListener('visibilitychange', () => {
    if (document.hidden && !pausado) alternarPausa();
});

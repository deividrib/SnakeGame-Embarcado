
# SnakeGame - Jogo da Cobrinha Embarcado

Este projeto consiste em um **jogo da cobrinha (Snake Game)** rodando em uma **placa BitDogLab** baseada na Raspberry Pi Pico W. O objetivo é demonstrar o uso de interfaces gráficas (Display OLED), controle por joystick, interrupções por botões, geração de sons e gravação de pontuações em um jogo embarcado.

---

## Descrição do Projeto

- **Joystick**: Controla a direção da cobrinha.
- **LED RGB (cor azul)**: Pisca toda vez que a cobrinha come uma comida.
- **Botão A (Pause)**: Pausa ou retoma o jogo.
- **Botão B (Som)**: Ativa ou desativa a música de fundo.
- **Teleporte nas paredes**: Se a cobrinha ultrapassar a borda do cenário, ela surge no lado oposto.
- **Game Over**: Se a cobrinha colidir com o próprio corpo, é exibida a tela de “Game Over”. Em seguida, o jogo pede para apertar o botão do joystick para reiniciar.
- **Placar de Pontuações**:
  - Se a pontuação atual for maior que algum recorde existente, surge uma tela informando “Novo recorde!” e pedindo para digitar o nome via Serial (aguarda até 8 segundos).  
  - Em seguida, exibe o placar com as três maiores pontuações e, ao pressionar novamente o botão do joystick, o jogo recomeça.  
  - Caso a pontuação não supere nenhum recorde, a tela de placar é mostrada diretamente após o “Game Over”.

---

## Estrutura do Projeto

A organização dos arquivos está dividida da seguinte forma:


```plaintext
 (raiz)
├── include/
│   ├── font.h                # Biblioteca com fontes para caracteres, números e símbolos
│   ├── highscore.h           # Protótipos de funções para gerenciamento do placar
│   ├── highscore.c           # Implementação das funções de placar
│   ├── matriz_led_control.h  # Protótipos de funções para controle da matriz de LEDs 5x5
│   ├── matriz_led_control.c  # Funções para controle da matriz de LEDs
│   ├── snake.h               # Protótipos de funções para o jogo da cobrinha
│   ├── snake.c               # Funções e configurações do jogo da cobrinha
│   ├── soun.h                # Protótipos de funções para efeitos sonoros
│   ├── soun.c                # Implementação dos efeitos sonoros
│   ├── ssd1306.h             # Protótipos de funções para manipulação do display OLED
│   └── ssd1306.c             # Funções para escrita e desenho no display OLED
├── SnakeGame.c               # Código principal do jogo
├── CMakeLists.txt            # Configuração do CMake para compilação
├── diagram.json              # Diagrama do projeto
├── pio_matrix.pio            # Código PIO para controle da matriz de LEDs
├── wokwi.toml                # Configuração para simulação no Wokwi
├── pico_sdk_import.cmake     # Importação do Pico SDK
├── .gitignore                # Arquivo para exclusões do Git
└── README.md                 # Este arquivo


```

## Requisitos e Componentes

### Hardware:
- **Placa BitDogLab (Raspberry Pi Pico W)** ou componentes discretos:
  - **Display OLED**: Utilizado para exibição de gráficos, textos e o placar.
  - **Joystick**: Para controlar a direção da cobrinha.
  - **Matriz de LEDs 5x5**: Para exibir efeitos visuais durante o jogo.
  - **LED RGB**: Utilizado para sinalizar a ação de comer a comida (pisca na cor azul).
  - **Botões**:
    - Botão A: Pausa/retoma o jogo.
    - Botão B: Ativa/desativa a música de fundo.
  - **Buzzer**: Para efeitos sonoros do jogo.

### Software:
- [Pico SDK](https://github.com/raspberrypi/pico-sdk)
- CMake
- Compilador ARM (ex.: arm-none-eabi-gcc)
- Ninja (opcional, mas recomendado)
- Ferramentas de simulação, como [Wokwi](https://wokwi.com/) (opcional)

### Pinagem do Projeto:

| Componente           | Pino (GPIO) |
|----------------------|------------|
| **Display OLED (I2C)** | |
| SDA                 | 14         |
| SCL                 | 15         |
| **Joystick**        | |
| Eixo X (VRX)       | 26         |
| Eixo Y (VRY)       | 27         |
| Botão do Joystick  | 22         |
| **Botões Adicionais** | |
| Botão A (Pausa)    | 5          |
| Botão B (Som)      | 6          |
| **LED RGB** (usado como azul) | 12 |
| **Matriz de LEDs 5x5** | 7 |
| **Buzzer** | 10, 21 |


## Funcionamento do Projeto

Este projeto é um jogo da cobrinha (Snake) desenvolvido para a **placa BitDogLab (Raspberry Pi Pico W)**, que já possui todos os componentes necessários embarcados. O jogo segue as seguintes mecânicas:

### Controles:
- **Joystick**: Controla a direção da cobrinha.
- **Botão A**: Pausa e retoma o jogo.
- **Botão B**: Ativa/desativa a música de fundo.
- **Botão do Joystick**: Confirma ações no jogo, como reiniciar após um **Game Over**.

### Regras do Jogo:
- O jogo inicia com a cobrinha se movendo automaticamente.
- **Comida aparece aleatoriamente** no mapa, e ao ser comida:
  - A cobrinha cresce.
  - O **LED RGB pisca na cor azul**.
  - A pontuação aumenta.
- **Movimentação nas bordas**:
  - Se a cobrinha ultrapassar uma borda, ela reaparece na posição oposta (efeito de teletransporte).
- **Colisão**:
  - Se a cobrinha colidir com o próprio corpo, o jogo exibe **"Game Over"** no **display OLED**.
  - O sistema então pede para pressionar o **botão do joystick** para reiniciar.

### Placar de Recordes:
- Se a pontuação for **maior que um dos três recordes armazenados**, aparece a mensagem **"Novo Recorde"**.
- O jogador deve **digitar seu nome via serial** dentro de um limite de 8 segundos.
- Em seguida, o sistema exibe **os três maiores recordes** antes de retornar ao jogo.

### Áudio:
- O jogo possui **efeitos sonoros e música de fundo**.
- A música pode ser ativada/desativada com o **Botão B**.

### Fluxo do Jogo:
1. O jogo inicia normalmente com a cobrinha em movimento.
2. O jogador controla a cobrinha usando o **joystick**.
3. Se a cobrinha comer a comida, cresce e o **LED RGB pisca azul**.
4. Caso atravesse a borda da tela, reaparece no lado oposto.
5. Se colidir com o próprio corpo:
   - Aparece **"Game Over"** no display OLED.
   - O jogo aguarda o **botão do joystick** ser pressionado.
   - Se for um **novo recorde**, solicita o nome via serial.
   - Exibe os **três melhores placares** antes de reiniciar o jogo.

O projeto faz uso da biblioteca **Pico SDK**, permitindo sua execução eficiente na **BitDogLab**.

## 📹 Demonstração em Vídeo
[![Assista ao vídeo](https://www.youtube.com/watch?v=lFmLZoSyaEw](https://www.youtube.com/watch?v=lFmLZoSyaEw)  


## 👤 Autor
**Deividson Ribeiro Silva**  
📧 [devidrs27@gmail.com](mailto:devidrs27@gmail.com)  
📧 [202011130033@gmail.com](mailto:202011130033@gmail.com)

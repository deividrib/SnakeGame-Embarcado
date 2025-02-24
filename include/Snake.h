#ifndef SNAKE_H
#define SNAKE_H

// Se ainda não estiver definida, define o pino do botão do joystick
#ifndef JOYSTICK_BTN
#define JOYSTICK_BTN 22
#endif

#include <stdbool.h>
#include <stdint.h>
#include "ssd1306.h"           // Certifique-se de que esta biblioteca esteja disponível
#include "matriz_led_control.h"

// Parâmetros da grade e do jogo
#define GRID_COLS 16
#define GRID_ROWS 8
#define CELL_SIZE 8
#define MAX_SNAKE_LENGTH (GRID_COLS * GRID_ROWS)

// Parâmetros do joystick
#define JOYSTICK_X_ADC 0
#define JOYSTICK_Y_ADC 1
#define JOYSTICK_CENTER 2048
#define DEAD_ZONE 100
#define NUM_SAMPLES 10

#define LED_B_PIN 12  // Pino do LED azul

// Parâmetro do delay entre frames (em milissegundos)
#define FRAME_DELAY 300

// Estrutura para representar uma posição na grade
typedef struct {
    int8_t x;
    int8_t y;
} Position;

// Enumeração para as direções
typedef enum {
    RIGHT = 0,
    DOWN,
    LEFT,
    UP
} Direction;

// Estrutura que encapsula o estado do jogo
typedef struct {
    Position snake[MAX_SNAKE_LENGTH];
    uint8_t snake_length;
    Direction current_direction;
    Position food;
    bool game_over_flag;
    int score;   // Campo adicionado para a pontuação do jogador
} SnakeGame;

// Protótipos das funções públicas da biblioteca
void snake_init(SnakeGame *game);
void snake_update_direction(SnakeGame *game);
void snake_update(SnakeGame *game, pio_t *led_matrix);
void snake_draw(SnakeGame *game, ssd1306_t *display);
void snake_game_over_screen(ssd1306_t *display, pio_t *led_matrix);
void food_eaten_animation();

#endif // SNAKE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "font.h"

// Configurações da grade do jogo (cada célula com 8x8 pixels)
#define GRID_COLS 16
#define GRID_ROWS 8
#define CELL_SIZE 8
#define MAX_SNAKE_LENGTH (GRID_COLS * GRID_ROWS)

// Configurações do joystick
#define JOYSTICK_X_ADC 0
#define JOYSTICK_Y_ADC 1
#define JOYSTICK_CENTER 2048
#define DEAD_ZONE 100
#define NUM_SAMPLES 10

// Tempo entre frames (em milissegundos)
#define FRAME_DELAY 300

// Pino para o botão do joystick (usado para reiniciar após Game Over)
#define JOYSTICK_BTN 22

// Estrutura que representa uma posição na grade
typedef struct {
    int8_t x;
    int8_t y;
} Position;

// Enum para as direções
typedef enum {
    RIGHT = 0,
    DOWN,
    LEFT,
    UP
} Direction;

// Variáveis globais do jogo
Position snake[MAX_SNAKE_LENGTH];
uint8_t snake_length;
Direction current_direction;
Position food;
bool game_over_flag = false;

// Instância do display SSD1306
ssd1306_t display;

// Protótipos das funções
void init_game(void);
void generate_food(void);
bool snake_collision(Position pos);
void update_game(void);
void draw_cell(uint8_t grid_x, uint8_t grid_y, bool fill);
void draw_game(void);
void update_direction(void);
void game_over_screen(void);

int main() {
    stdio_init_all();
    sleep_ms(2000);  // Aguarda 2s para estabilização

    // Inicializa a interface I2C para o display OLED (pinos SDA=14, SCL=15)
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);

    ssd1306_init(&display, 128, 64, false, 0x3C, i2c1);
    ssd1306_config(&display);

    // Inicializa ADC para os eixos do joystick (GPIO26 e GPIO27)
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);

    // Inicializa o botão do joystick para reiniciar (GPIO22)
    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);

    // Semente para números aleatórios (para a comida)
    srand(time_us_32());

    // Inicializa o jogo
    init_game();

    while (true) {
        update_direction();
        update_game();
        draw_game();

        if (game_over_flag) {
            game_over_screen();
            init_game();
        }

        sleep_ms(FRAME_DELAY);
    }
    
    return 0;
}

// Inicializa o estado do jogo: cobra, direção e gera a comida
void init_game(void) {
    snake_length = 3;
    // Posiciona a cobra no centro da grade
    snake[0].x = GRID_COLS / 2;
    snake[0].y = GRID_ROWS / 2;
    snake[1].x = snake[0].x - 1;
    snake[1].y = snake[0].y;
    snake[2].x = snake[0].x - 2;
    snake[2].y = snake[0].y;
    
    current_direction = RIGHT;
    game_over_flag = false;
    generate_food();
}

// Gera uma posição aleatória para a comida, garantindo que não coincida com a cobra
void generate_food(void) {
    bool valid = false;
    Position pos;
    while (!valid) {
        pos.x = rand() % GRID_COLS;
        pos.y = rand() % GRID_ROWS;
        valid = true;
        for (int i = 0; i < snake_length; i++) {
            if (snake[i].x == pos.x && snake[i].y == pos.y) {
                valid = false;
                break;
            }
        }
    }
    food = pos;
}

// Verifica se a posição colide com o corpo da cobra
bool snake_collision(Position pos) {
    for (int i = 0; i < snake_length; i++) {
        if (snake[i].x == pos.x && snake[i].y == pos.y)
            return true;
    }
    return false;
}

// Atualiza o estado do jogo: movimenta a cobra e trata a lógica de crescimento e colisão
void update_game(void) {
    Position new_head = snake[0];

    // Calcula a nova posição da cabeça com base na direção
    if (current_direction == RIGHT) new_head.x++;
    else if (current_direction == DOWN) new_head.y++;
    else if (current_direction == LEFT) new_head.x--;
    else if (current_direction == UP) new_head.y--;

    // Implementa o wrap-around (quando sai de um lado, entra pelo outro)
    if (new_head.x >= GRID_COLS) new_head.x = 0;
    else if (new_head.x < 0) new_head.x = GRID_COLS - 1;
    if (new_head.y >= GRID_ROWS) new_head.y = 0;
    else if (new_head.y < 0) new_head.y = GRID_ROWS - 1;

    // Verifica colisão com o próprio corpo
    if (snake_collision(new_head)) {
        game_over_flag = true;
        return;
    }

    // Verifica se a comida foi alcançada
    bool ate_food = (new_head.x == food.x && new_head.y == food.y);

    // Atualiza as posições da cobra (shift de posições)
    for (int i = snake_length; i > 0; i--) {
        snake[i] = snake[i - 1];
    }
    snake[0] = new_head;

    if (ate_food) {
        if (snake_length < MAX_SNAKE_LENGTH) {
            snake_length++;
        }
        generate_food();
    }
}

// Desenha uma célula na grade (a célula pode ser preenchida ou apenas o contorno)
void draw_cell(uint8_t grid_x, uint8_t grid_y, bool fill) {
    uint8_t x_pixel = grid_x * CELL_SIZE;
    uint8_t y_pixel = grid_y * CELL_SIZE;
    ssd1306_rect(&display, y_pixel, x_pixel, CELL_SIZE, CELL_SIZE, 1, fill);
}

// Desenha o estado atual do jogo no display OLED
void draw_game(void) {
    ssd1306_fill(&display, 0);
    
    // Desenha a comida (preenchida)
    draw_cell(food.x, food.y, true);
    
    // Desenha a cobra (cada célula preenchida)
    for (int i = 0; i < snake_length; i++) {
        draw_cell(snake[i].x, snake[i].y, true);
    }
    
    ssd1306_send_data(&display);
}

// Atualiza a direção da cobra com base nas leituras do joystick
// Utiliza o mapeamento dos ADCs (troca dos canais) e aplica a zona morta.
// A lógica para o eixo Y foi invertida para corrigir a inversão.
void update_direction(void) {
    uint32_t total_x = 0, total_y = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        adc_select_input(JOYSTICK_X_ADC);
        total_x += adc_read();
        adc_select_input(JOYSTICK_Y_ADC);
        total_y += adc_read();
        sleep_ms(1);
    }
    // Troca de leituras: o canal X é usado para o eixo Y e vice-versa
    uint16_t adc_y = total_x / NUM_SAMPLES; // Canal X para eixo Y
    uint16_t adc_x = total_y / NUM_SAMPLES; // Canal Y para eixo X

    // Aplica a zona morta
    if (abs((int)adc_x - JOYSTICK_CENTER) < DEAD_ZONE) {
        adc_x = JOYSTICK_CENTER;
    }
    if (abs((int)adc_y - JOYSTICK_CENTER) < DEAD_ZONE) {
        adc_y = JOYSTICK_CENTER;
    }

    int16_t diff_x = (int16_t)adc_x - JOYSTICK_CENTER;
    int16_t diff_y = (int16_t)adc_y - JOYSTICK_CENTER;

    // Atualiza a direção baseada na maior variação, evitando reversão direta.
    // Para o eixo Y, as atribuições foram invertidas para corrigir a inversão.
    if (abs(diff_x) > abs(diff_y)) {
        if (diff_x > DEAD_ZONE && current_direction != LEFT) {
            current_direction = RIGHT;
        } else if (diff_x < -DEAD_ZONE && current_direction != RIGHT) {
            current_direction = LEFT;
        }
    } else {
        if (diff_y > DEAD_ZONE && current_direction != DOWN) {
            current_direction = UP;
        } else if (diff_y < -DEAD_ZONE && current_direction != UP) {
            current_direction = DOWN;
        }
    }
}

// Exibe uma tela de "Game Over" e aguarda o pressionamento do botão para reiniciar
void game_over_screen(void) {
    ssd1306_fill(&display, 0);
    ssd1306_draw_string(&display, "GAME OVER", 20, 20);
    ssd1306_draw_string(&display, "Press BTN", 20, 40);
    ssd1306_send_data(&display);

    // Aguarda até que o botão do joystick seja pressionado e depois liberado
    while (gpio_get(JOYSTICK_BTN)) {
        sleep_ms(100);
    }
    while (!gpio_get(JOYSTICK_BTN)) {
        sleep_ms(100);
    }
}

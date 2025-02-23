#include "snake.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <stdlib.h>
#include "hardware/pwm.h"

#define BITMAP_SIZE 8  // Supondo que CELL_SIZE seja 8

// -------------------------------------------------------------------
// Bitmaps para o novo design:

// Bitmap da cabeça da cobra (com "olhos")
static const uint8_t snake_head_bitmap[BITMAP_SIZE * BITMAP_SIZE] = {
    0,0,1,1,1,1,0,0,
    0,1,1,1,1,1,1,0,
    1,1,0,1,1,0,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,0,1,1,0,1,1,
    0,1,1,0,0,1,1,0,
    0,0,1,1,1,1,0,0
};

// Bitmap para o corpo da cobra
static const uint8_t snake_body_bitmap[BITMAP_SIZE * BITMAP_SIZE] = {
    0,0,1,1,1,1,0,0,
    0,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    0,1,1,1,1,1,1,0,
    0,0,1,1,1,1,0,0
};

// Bitmap para a cauda da cobra (com final afinado)
static const uint8_t snake_tail_bitmap[BITMAP_SIZE * BITMAP_SIZE] = {
    0,0,1,1,1,1,0,0,
    0,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    0,1,1,1,1,1,1,0,
    0,0,1,1,1,1,0,0,
    0,0,0,1,1,0,0,0
};

// Bitmap para o alimento (desenhado em formato de losango)
static const uint8_t food_bitmap[BITMAP_SIZE * BITMAP_SIZE] = {
    0,0,0,1,1,0,0,0,
    0,0,1,1,1,1,0,0,
    0,1,1,1,1,1,1,0,
    1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,
    0,1,1,1,1,1,1,0,
    0,0,1,1,1,1,0,0,
    0,0,0,1,1,0,0,0
};

// -------------------------------------------------------------------
// Funções internas para controle do jogo

// Gera uma posição aleatória para a comida, evitando sobrepor a cobra.
static void snake_generate_food(SnakeGame *game) {
    bool valid = false;
    Position pos;
    while (!valid) {
        pos.x = rand() % GRID_COLS;
        pos.y = rand() % GRID_ROWS;
        valid = true;
        for (int i = 0; i < game->snake_length; i++) {
            if (game->snake[i].x == pos.x && game->snake[i].y == pos.y) {
                valid = false;
                break;
            }
        }
    }
    game->food = pos;
}

// Verifica se a posição informada colide com algum segmento da cobra.
static bool snake_collision(SnakeGame *game, Position pos) {
    for (int i = 0; i < game->snake_length; i++) {
        if (game->snake[i].x == pos.x && game->snake[i].y == pos.y)
            return true;
    }
    return false;
}

// Inicializa o estado do jogo.
void snake_init(SnakeGame *game) {
    game->snake_length = 3;
    // Posiciona a cobra no centro da grade.
    game->snake[0].x = GRID_COLS / 2;
    game->snake[0].y = GRID_ROWS / 2;
    game->snake[1].x = game->snake[0].x - 1;
    game->snake[1].y = game->snake[0].y;
    game->snake[2].x = game->snake[0].x - 2;
    game->snake[2].y = game->snake[0].y;
    
    game->current_direction = RIGHT;
    game->game_over_flag = false;
    snake_generate_food(game);
}

// Atualiza a direção da cobra com base na leitura dos ADCs do joystick.
// Agora, cada canal é lido sem troca: o canal X controla o eixo horizontal e o canal Y o vertical.
void snake_update_direction(SnakeGame *game) {
    uint32_t total_x = 0, total_y = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        adc_select_input(JOYSTICK_X_ADC);
        total_x += adc_read();
        adc_select_input(JOYSTICK_Y_ADC);
        total_y += adc_read();
        sleep_ms(1);
    }
    // Realiza a troca: canal X → eixo Y, canal Y → eixo X
    uint16_t adc_y = total_x / NUM_SAMPLES;
    uint16_t adc_x = total_y / NUM_SAMPLES;
    
    // Aplica a zona morta
    if (abs((int)adc_x - JOYSTICK_CENTER) < DEAD_ZONE)
        adc_x = JOYSTICK_CENTER;
    if (abs((int)adc_y - JOYSTICK_CENTER) < DEAD_ZONE)
        adc_y = JOYSTICK_CENTER;
    
    int16_t diff_x = (int16_t)adc_x - JOYSTICK_CENTER;
    int16_t diff_y = (int16_t)adc_y - JOYSTICK_CENTER;
    
    // Atualiza a direção baseada na maior variação, evitando reversão direta.
    if (abs(diff_x) > abs(diff_y)) {
        if (diff_x > DEAD_ZONE && game->current_direction != LEFT)
            game->current_direction = RIGHT;
        else if (diff_x < -DEAD_ZONE && game->current_direction != RIGHT)
            game->current_direction = LEFT;
    } else {
        // Para o eixo Y, inverte as condições para corrigir a inversão:
        if (diff_y > DEAD_ZONE && game->current_direction != DOWN)
            game->current_direction = UP;
        else if (diff_y < -DEAD_ZONE && game->current_direction != UP)
            game->current_direction = DOWN;
    }
}
// Atualiza o estado do jogo: movimenta a cobra, trata alimentação, wrap-around e colisões.
void snake_update(SnakeGame *game, pio_t *led_matrix) {
    Position new_head = game->snake[0];

    // Calcula a nova posição com base na direção atual.
    if (game->current_direction == RIGHT)
        new_head.x++;
    else if (game->current_direction == DOWN)
        new_head.y++;
    else if (game->current_direction == LEFT)
        new_head.x--;
    else if (game->current_direction == UP)
        new_head.y--;

    // Wrap-around: se ultrapassar a borda, reaparece do outro lado.
    if (new_head.x >= GRID_COLS) new_head.x = 0;
    else if (new_head.x < 0) new_head.x = GRID_COLS - 1;
    if (new_head.y >= GRID_ROWS) new_head.y = 0;
    else if (new_head.y < 0) new_head.y = GRID_ROWS - 1;

    // Verifica colisão com o corpo.
    if (snake_collision(game, new_head)) {
        game->game_over_flag = true;
        return;
    }

    bool ate_food = (new_head.x == game->food.x && new_head.y == game->food.y);

    // Move a cobra (shift dos segmentos).
    for (int i = game->snake_length; i > 0; i--) {
        game->snake[i] = game->snake[i - 1];
    }
    game->snake[0] = new_head;

    if (ate_food) {
        if (game->snake_length < MAX_SNAKE_LENGTH)
            game->snake_length++;
        
        food_eaten_animation();  // Efeito visual com LED azul.
        snake_generate_food(game);
    }
}

// -------------------------------------------------------------------
// Funções de desenho com o novo design

// Converte coordenadas da grade para pixels e desenha um bitmap no display.
// x_pixel representa a coluna e y_pixel a linha.
static void draw_bitmap(ssd1306_t *display, uint8_t x_pixel, uint8_t y_pixel, const uint8_t *bitmap, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (bitmap[i * size + j])
                ssd1306_pixel(display, x_pixel + j, y_pixel + i, 1);
        }
    }
}

// Desenha o estado atual do jogo utilizando os bitmaps personalizados.
void snake_draw(SnakeGame *game, ssd1306_t *display) {
    ssd1306_fill(display, 0);
    
    // Desenha o alimento com o novo design.
    uint8_t food_x = game->food.x * CELL_SIZE;
    uint8_t food_y = game->food.y * CELL_SIZE;
    draw_bitmap(display, food_x, food_y, food_bitmap, BITMAP_SIZE);
    
    // Desenha cada segmento da cobra com o bitmap correspondente.
    for (int i = 0; i < game->snake_length; i++) {
        uint8_t seg_x = game->snake[i].x * CELL_SIZE;
        uint8_t seg_y = game->snake[i].y * CELL_SIZE;
        
        if (i == 0)
            draw_bitmap(display, seg_x, seg_y, snake_head_bitmap, BITMAP_SIZE);
        else if (i == game->snake_length - 1)
            draw_bitmap(display, seg_x, seg_y, snake_tail_bitmap, BITMAP_SIZE);
        else
            draw_bitmap(display, seg_x, seg_y, snake_body_bitmap, BITMAP_SIZE);
    }
    
    ssd1306_send_data(display);
}

// -------------------------------------------------------------------
// Tela de "Game Over" e animação de LED (mantidas da base)

void snake_game_over_screen(ssd1306_t *display, pio_t *led_matrix) {
    ssd1306_fill(display, 0);
    ssd1306_draw_string(display, "GAME OVER", 20, 20);
    ssd1306_draw_string(display, "Press BTN", 20, 40);
    ssd1306_send_data(display);
    
    double x_pattern[25] = {
         1.0, 0.0, 0.0, 0.0, 1.0,
         0.0, 1.0, 0.0, 1.0, 0.0,
         0.0, 0.0, 1.0, 0.0, 0.0,
         0.0, 1.0, 0.0, 1.0, 0.0,
         1.0, 0.0, 0.0, 0.0, 1.0
    };
    
    led_matrix->r = 1.0;
    led_matrix->g = 0.0;
    led_matrix->b = 0.0;
    
    for (int i = 0; i < 5; i++) {
        desenho_pio_rgb(x_pattern, led_matrix);
        sleep_ms(500);
        desliga_tudo(led_matrix);
        sleep_ms(500);
    }
    
    while (gpio_get(JOYSTICK_BTN)) {
        sleep_ms(100);
    }
    while (!gpio_get(JOYSTICK_BTN)) {
        sleep_ms(100);
    }
}

void food_eaten_animation() {
    uint slice_b = pwm_gpio_to_slice_num(LED_B_PIN);
    uint chan_b = pwm_gpio_to_channel(LED_B_PIN);
    uint brightness = 255;
    pwm_set_chan_level(slice_b, chan_b, brightness);
    sleep_ms(200);
    pwm_set_chan_level(slice_b, chan_b, 0);
}

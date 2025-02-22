#include "snake.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include <stdlib.h>
#include "hardware/pwm.h"


// Função interna para gerar a posição da comida em um local válido
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

// Função interna que verifica se a posição colide com o corpo da cobra
static bool snake_collision(SnakeGame *game, Position pos) {
    for (int i = 0; i < game->snake_length; i++) {
        if (game->snake[i].x == pos.x && game->snake[i].y == pos.y)
            return true;
    }
    return false;
}

// Inicializa o estado do jogo
void snake_init(SnakeGame *game) {
    game->snake_length = 3;
    // Posiciona a cobra no centro da grade
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
// Utiliza a troca dos canais (canal X → eixo Y, canal Y → eixo X) e aplica a zona morta.
// Para o eixo Y, inverte a lógica para corrigir a inversão.
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

// Atualiza o estado do jogo: movimenta a cobra, trata alimentação, wrap-around e colisões com o corpo
// Em snake.c, na função snake_update:
void snake_update(SnakeGame *game, pio_t *led_matrix) {
    Position new_head = game->snake[0];

    // Calcula a nova posição com base na direção atual
    if (game->current_direction == RIGHT)
        new_head.x++;
    else if (game->current_direction == DOWN)
        new_head.y++;
    else if (game->current_direction == LEFT)
        new_head.x--;
    else if (game->current_direction == UP)
        new_head.y--;

    // Wrap-around
    if (new_head.x >= GRID_COLS) new_head.x = 0;
    else if (new_head.x < 0) new_head.x = GRID_COLS - 1;
    if (new_head.y >= GRID_ROWS) new_head.y = 0;
    else if (new_head.y < 0) new_head.y = GRID_ROWS - 1;

    // Verifica colisão com o corpo
    if (snake_collision(game, new_head)) {
        game->game_over_flag = true;
        return;
    }

    bool ate_food = (new_head.x == game->food.x && new_head.y == game->food.y);

    // Move a cobra (shift)
    for (int i = game->snake_length; i > 0; i--) {
        game->snake[i] = game->snake[i - 1];
    }
    game->snake[0] = new_head;

    if (ate_food) {
        if (game->snake_length < MAX_SNAKE_LENGTH)
            game->snake_length++;
        
        food_eaten_animation();  // Chama o efeito de luz branca no LED RGB
        snake_generate_food(game);
    }
    
}

// Função interna para desenhar uma célula na grade
static void draw_cell(uint8_t grid_x, uint8_t grid_y, bool fill, ssd1306_t *display) {
    uint8_t x_pixel = grid_x * CELL_SIZE;
    uint8_t y_pixel = grid_y * CELL_SIZE;
    ssd1306_rect(display, y_pixel, x_pixel, CELL_SIZE, CELL_SIZE, 1, fill);
}

// Desenha o estado atual do jogo no display OLED
void snake_draw(SnakeGame *game, ssd1306_t *display) {
    ssd1306_fill(display, 0);
    
    // Desenha a comida
    draw_cell(game->food.x, game->food.y, true, display);
    
    // Desenha cada célula da cobra
    for (int i = 0; i < game->snake_length; i++) {
        draw_cell(game->snake[i].x, game->snake[i].y, true, display);
    }
    
    ssd1306_send_data(display);
}

// Exibe uma tela de "Game Over" e aguarda o pressionamento do botão do joystick para reiniciar
void snake_game_over_screen(ssd1306_t *display, pio_t *led_matrix) {
    // Exibe mensagem de game over no OLED
    ssd1306_fill(display, 0);
    ssd1306_draw_string(display, "GAME OVER", 20, 20);
    ssd1306_draw_string(display, "Press BTN", 20, 40);
    ssd1306_send_data(display);
    
    // Define o padrão "X" para a matriz LED 5x5
    double x_pattern[25] = {
         1.0, 0.0, 0.0, 0.0, 1.0,
         0.0, 1.0, 0.0, 1.0, 0.0,
         0.0, 0.0, 1.0, 0.0, 0.0,
         0.0, 1.0, 0.0, 1.0, 0.0,
         1.0, 0.0, 0.0, 0.0, 1.0
    };
    
    // Configura a cor desejada: vermelho (r = 1.0, g = 0.0, b = 0.0)
    led_matrix->r = 1.0;
    led_matrix->g = 0.0;
    led_matrix->b = 0.0;
    
    // Efeito de piscar o "X"
    for (int i = 0; i < 5; i++) {
        desenho_pio_rgb(x_pattern, led_matrix);  // Envia o padrão para a matriz com a cor definida
        sleep_ms(500);
        desliga_tudo(led_matrix);                // Apaga a matriz
        sleep_ms(500);
    }
    
    // Aguarda o pressionamento do botão para reiniciar
    while (gpio_get(JOYSTICK_BTN)) {
        sleep_ms(100);
    }
    while (!gpio_get(JOYSTICK_BTN)) {
        sleep_ms(100);
    }
}

// Função que exibe uma animação de flash ao comer,
void food_eaten_animation() {
    uint slice_b = pwm_gpio_to_slice_num(LED_B_PIN);
    uint chan_b = pwm_gpio_to_channel(LED_B_PIN);
    uint brightness = 255;  // Brilho máximo para o LED azul

    // Liga o LED azul
    pwm_set_chan_level(slice_b, chan_b, brightness);
    
    // Mantém o efeito por um tempo curto
    sleep_ms(200);

    // Desliga o LED azul
    pwm_set_chan_level(slice_b, chan_b, 0);
}

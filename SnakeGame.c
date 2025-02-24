#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "snake.h"
#include "sound.h"
#include "matriz_led_control.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include <string.h>

#define LED_B_PIN 12    // Usado apenas o LED azul
#define JOYSTICK_BTN 22
#define LED_MATRIX_PIN 7

#define PAUSE_BTN 5     // Botão A para pausar (GPIO 5)
#define SOUND_BTN 6     // Botão B para mutar/desmutar som (GPIO 6)

#define MAX_HIGH_SCORES 3
#define MAX_NAME_LENGTH 16

typedef struct {
    int score;
    char name[MAX_NAME_LENGTH];
} HighScore;

HighScore high_scores[MAX_HIGH_SCORES];

/// Inicializa o placar com valores padrão.
void init_high_scores() {
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        high_scores[i].score = 0;
        strcpy(high_scores[i].name, "---");
    }
}


// Atualiza o placar se a pontuação atual for um novo recorde.
// Exibe uma mensagem no OLED e solicita a entrada do nome via Serial ou via botão.
void update_high_scores(ssd1306_t *display, int score) {
    if (score > high_scores[MAX_HIGH_SCORES - 1].score) {
        // Define o tamanho do array para 8 caracteres + 1 para o '\0'
        char name[9] = {0};

        // Exibe mensagem no OLED
        ssd1306_fill(display, 0);
        ssd1306_draw_string(display, "Novo recorde!", 10, 10);
        ssd1306_draw_string(display, "Dgt seu nome:", 10, 25);
        ssd1306_draw_string(display, "via Serial", 10, 40);
        ssd1306_draw_string(display, "Aguarde 8s", 10, 55);
        ssd1306_send_data(display);

        // Informa via Serial
        printf("Novo recorde! Insira seu nome (max 8 caracteres):\n");
        fflush(stdout);

        int index = 0;
        int elapsed = 0;
        // Aguarda até 8000 ms (8 s) para entrada do usuário, verificando a cada 10 ms
        while (elapsed < 8000) {
            int ch = getchar_timeout_us(10000); // timeout de 10 ms
            if (ch != PICO_ERROR_TIMEOUT) {
                if (ch == '\n' || ch == '\r') {
                    break;
                }
                if (index < 8) {  // Limita a 8 caracteres
                    name[index++] = (char)ch;
                }
            }
            sleep_ms(10);
            elapsed += 10;
        }
        // Se nenhum caractere foi digitado, define como "Anonimo"
        if (index == 0) {
            strcpy(name, "Anonimo");
        } else {
            name[index] = '\0';
        }

        // Insere o novo recorde na posição correta (mantendo a ordem decrescente)
        int pos = MAX_HIGH_SCORES - 1;
        while (pos > 0 && score > high_scores[pos - 1].score) {
            high_scores[pos] = high_scores[pos - 1];
            pos--;
        }
        high_scores[pos].score = score;
        strncpy(high_scores[pos].name, name, 9);
        high_scores[pos].name[8] = '\0';

        // Exibe mensagem de confirmação via Serial
        printf("Nome registrado: %s-%dpontos\n", name, score);
        fflush(stdout);
    }
}



// Exibe o placar na tela OLED.
void display_scoreboard(ssd1306_t *display) {
    char buffer[32];
    ssd1306_fill(display, 0);
    ssd1306_draw_string(display, "Placar", 30, 0);
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        snprintf(buffer, sizeof(buffer), "%d. %s - %d", i + 1, high_scores[i].name, high_scores[i].score);
        ssd1306_draw_string(display, buffer, 0, 10 + i * 10);
    }
    ssd1306_draw_string(display, "Aperte BTN para jogar", 0, 50);
    ssd1306_send_data(display);
}

// Variáveis globais de estado
volatile bool game_paused = false;
volatile bool game_sound_enabled = true;

// Variáveis para debouncing (em microsegundos)
volatile uint32_t last_pause_interrupt_time = 0;
volatile uint32_t last_sound_interrupt_time = 0;
#define DEBOUNCE_TIME 200000 // 200 ms

// Callback de interrupção para PAUSE_BTN e SOUND_BTN com debouncing
void gpio_callback(uint gpio, uint32_t events) {
    uint32_t current_time = time_us_32();
    
    if (gpio == PAUSE_BTN && (events & GPIO_IRQ_EDGE_FALL)) {
        if (current_time - last_pause_interrupt_time < DEBOUNCE_TIME)
            return;
        last_pause_interrupt_time = current_time;
        game_paused = !game_paused;
    }
    
    if (gpio == SOUND_BTN && (events & GPIO_IRQ_EDGE_FALL)) {
        if (current_time - last_sound_interrupt_time < DEBOUNCE_TIME)
            return;
        last_sound_interrupt_time = current_time;
        game_sound_enabled = !game_sound_enabled;
    }
}

void setup_blue_led() {
    gpio_set_function(LED_B_PIN, GPIO_FUNC_PWM);
    uint slice_b = pwm_gpio_to_slice_num(LED_B_PIN);
    uint chan_b = pwm_gpio_to_channel(LED_B_PIN);
    pwm_set_wrap(slice_b, 255);
    pwm_set_enabled(slice_b, true);
}

int main() {
    stdio_init_all();
    setvbuf(stdin, NULL, _IONBF, 0);

    sleep_ms(2000);

    setup_blue_led();

    // Inicializa o display OLED via I2C (SDA=14, SCL=15)
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
    ssd1306_t display;
    ssd1306_init(&display, 128, 64, false, 0x3C, i2c1);
    ssd1306_config(&display);

    // Inicializa o ADC para o joystick (GPIO26 e GPIO27)
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);

    // Inicializa o botão do joystick (GPIO22)
    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);

    // Inicializa o botão de pausa (GPIO5)
    gpio_init(PAUSE_BTN);
    gpio_set_dir(PAUSE_BTN, GPIO_IN);
    gpio_pull_up(PAUSE_BTN);
    // Habilita a interrupção para PAUSE_BTN na borda de descida
    gpio_set_irq_enabled_with_callback(PAUSE_BTN, GPIO_IRQ_EDGE_FALL, true, gpio_callback);

    // Inicializa o botão de som (GPIO6)
    gpio_init(SOUND_BTN);
    gpio_set_dir(SOUND_BTN, GPIO_IN);
    gpio_pull_up(SOUND_BTN);
    // Habilita a interrupção para SOUND_BTN na borda de descida
    gpio_set_irq_enabled(SOUND_BTN, GPIO_IRQ_EDGE_FALL, true);

    // Inicializa a matriz de LEDs
    pio_t led_matrix;
    led_matrix.pio = pio0;
    init_pio_routine(&led_matrix, LED_MATRIX_PIN);

    srand(time_us_32());

      // Inicializa o placar
      init_high_scores();

    SnakeGame game;
    snake_init(&game);

    while (true) {
        if (game_paused) {
            ssd1306_fill(&display, 0);
            ssd1306_draw_string(&display, "PAUSE", 44, 28);
            ssd1306_send_data(&display);
            sleep_ms(100);
            continue;
        }

        snake_update_direction(&game);
        snake_update(&game, &led_matrix);
        snake_draw(&game, &display);

        if (game_sound_enabled) {
            sound_play_background_note();
        }

        if (game.game_over_flag) {
            if (game_sound_enabled) {
                sound_play_explosion_sound();
            }
            // Exibe a tela de Game Over (mantendo a animação já implementada)
            snake_game_over_screen(&display, &led_matrix);

            // Verifica se o jogador obteve um novo recorde e, se sim, atualiza o placar.
           update_high_scores(&display, game.score);

            // Exibe o placar atualizado
            display_scoreboard(&display);

            // Aguarda até o botão do joystick (GPIO22) ser pressionado para reiniciar
            while (gpio_get(JOYSTICK_BTN)) {
                sleep_ms(100);
            }
            while (!gpio_get(JOYSTICK_BTN)) {
                sleep_ms(100);
            }
            // Reinicia o jogo
            snake_init(&game);
        }
        
        sleep_ms(FRAME_DELAY);
    }
    
    return 0;
}
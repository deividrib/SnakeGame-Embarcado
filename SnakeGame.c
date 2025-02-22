#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "snake.h"
#include "sound.h"
#include "matriz_led_control.h"
#include "hardware/pwm.h"


#define LED_B_PIN 12    // Usado apenas o LED azul
#define JOYSTICK_BTN 22
#define LED_MATRIX_PIN 7
#define PAUSE_BTN 5     // Botão A conectado ao GPIO 5

// Variável global para indicar se o jogo está pausado
volatile bool game_paused = false;
// Variável global para debouncing
volatile uint32_t last_interrupt_time = 0;
#define DEBOUNCE_TIME 50000 // 50ms em microsegundos

// Callback de interrupção para o botão de pausa com debouncing
void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == PAUSE_BTN && (events & GPIO_IRQ_EDGE_FALL)) {
        uint32_t current_time = time_us_32();
        if (current_time - last_interrupt_time < DEBOUNCE_TIME) {
            return; // Ignora se o intervalo for menor que o tempo de debouncing
        }
        last_interrupt_time = current_time;
        game_paused = !game_paused;
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
    // Configura a interrupção com callback para o botão de pausa na borda de descida
    gpio_set_irq_enabled_with_callback(PAUSE_BTN, GPIO_IRQ_EDGE_FALL, true, gpio_callback);

    // Inicializa a matriz de LEDs
    pio_t led_matrix;
    led_matrix.pio = pio0;
    init_pio_routine(&led_matrix, LED_MATRIX_PIN);

    srand(time_us_32());

    SnakeGame game;
    snake_init(&game);

    while (true) {
        // Se o jogo estiver pausado, exibe a mensagem "PAUSE" e não atualiza o jogo
        if (game_paused) {
            ssd1306_fill(&display, 0);
            ssd1306_draw_string(&display, "PAUSE", 30, 30);
            ssd1306_send_data(&display);
            sleep_ms(100);
            continue;
        }

        snake_update_direction(&game);
        snake_update(&game, &led_matrix);
        snake_draw(&game, &display);

        sound_play_background_note();

        if (game.game_over_flag) {
            sound_play_explosion_sound();
            snake_game_over_screen(&display, &led_matrix);
            snake_init(&game);
        }
        
        sleep_ms(FRAME_DELAY);
    }
    
    return 0;
}

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "snake.h"
#include "sound.h"
#include "matriz_led_control.h"  // Biblioteca para a matriz de LEDs
#include "hardware/pwm.h"

#define LED_B_PIN 12  // Pino do LED azul


#define JOYSTICK_BTN 22
#define LED_MATRIX_PIN 7       // Pino onde a matriz está conectada

void setup_rgb_led() {

 
  gpio_set_function(LED_B_PIN, GPIO_FUNC_PWM);

  uint slice_b = pwm_gpio_to_slice_num(LED_B_PIN);
  uint chan_b = pwm_gpio_to_channel(LED_B_PIN);
  
    pwm_set_wrap(slice_b, 255);

    pwm_set_enabled(slice_b, true);
}


int main() {
    stdio_init_all();
    sleep_ms(2000);

    setup_rgb_led();  // Inicializa o LED RGB com PWM

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

    // Inicializa a matriz de LEDs
    pio_t led_matrix;
    led_matrix.pio = pio0;  // Use o PIO correto (pio0 ou pio1)
    init_pio_routine(&led_matrix, LED_MATRIX_PIN);

    // Semente para números aleatórios
    srand(time_us_32());

    // Inicializa o jogo
    SnakeGame game;
    snake_init(&game);

    while (true) {
        snake_update_direction(&game);
        // Agora passamos o ponteiro da matriz para a função de atualização:
        snake_update(&game, &led_matrix);
        snake_draw(&game, &display);

        // Toca a nota de fundo a cada ciclo
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

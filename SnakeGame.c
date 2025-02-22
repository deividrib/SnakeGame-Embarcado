#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "snake.h"
#include "sound.h"

#define JOYSTICK_BTN 22

int main() {
    stdio_init_all();
    sleep_ms(2000);

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

    // Semente para números aleatórios
    srand(time_us_32());

    // Inicializa o jogo
    SnakeGame game;
    snake_init(&game);

    while (true) {
        snake_update_direction(&game);
        snake_update(&game);
        snake_draw(&game, &display);

        // Toca a nota de fundo a cada ciclo
        sound_play_background_note();

        if (game.game_over_flag) {
            sound_play_explosion_sound();
            snake_game_over_screen(&display);
            snake_init(&game);
        }
        
        sleep_ms(FRAME_DELAY);
    }
    
    return 0;
}

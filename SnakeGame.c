#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "snake.h"
#include "hardware/clocks.h"

// Definições de pinos para os buzzers
#define BUZZER_BG 10   // Buzzer para música de fundo (GPIO10)
#define BUZZER_EXP 21  // Buzzer para som de explosão (GPIO21)

// Configuração do botão do joystick já definida na biblioteca snake.h

// --- Funções para tocar tons via PWM ---

// Defina uma estrutura para representar uma nota
typedef struct {
    uint32_t frequency;
    uint32_t duration_ms;
} note_t;

// Uma melodia mais animada: uma sequência de notas (por exemplo, uma escala rítmica)
note_t background_melody[] = {
    {523, 100},  // C5
    {587, 100},  // D5
    {659, 100},  // E5
    {698, 100},  // F5
    {784, 100},  // G5
    {880, 100},  // A5
    {988, 100},  // B5
    {1047, 200}, // C6 (nota mais longa)
    {988, 100},
    {880, 100},
    {784, 100},
    {698, 100},
    {659, 100},
    {587, 100},
    {523, 200}   // Finalizando com C5 prolongado
};
size_t background_melody_length = sizeof(background_melody) / sizeof(background_melody[0]);
static size_t current_note_index = 0;

// Função para tocar um tom (como antes)
void play_tone(uint gpio, uint32_t frequency, uint32_t duration_ms) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint channel = pwm_gpio_to_channel(gpio);
    uint32_t clock = clock_get_hz(clk_sys);
    uint32_t wrap = clock / frequency - 1;
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, channel, wrap / 2);  // 50% duty cycle
    pwm_set_enabled(slice_num, true);
    sleep_ms(duration_ms);
    pwm_set_enabled(slice_num, false);
}

// Nova função para tocar a melodia de fundo
void play_background_note() {
    note_t note = background_melody[current_note_index];
    play_tone(BUZZER_BG, note.frequency, note.duration_ms);
    current_note_index = (current_note_index + 1) % background_melody_length;
}


// Som de explosão: uma sequência de tons descendentes
void play_explosion_sound() {
    uint32_t explosion[] = {600, 500, 400, 300, 200};
    size_t explosion_length = sizeof(explosion) / sizeof(explosion[0]);
    for (size_t i = 0; i < explosion_length; i++) {
         play_tone(BUZZER_EXP, explosion[i], 50);
    }
}

// --- Código principal do jogo ---
int main() {
    stdio_init_all();
    sleep_ms(2000);  // Tempo para estabilização

    // Inicializa o display OLED via I2C (pinos SDA=14, SCL=15)
    i2c_init(i2c1, 100 * 1000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
    ssd1306_t display;
    ssd1306_init(&display, 128, 64, false, 0x3C, i2c1);
    ssd1306_config(&display);

    // Inicializa o ADC para os eixos do joystick (GPIO26 e GPIO27)
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);

    // Inicializa o botão do joystick (GPIO22)
    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);

    // Semente para números aleatórios (para gerar a comida)
    srand(time_us_32());

    // Inicializa o jogo utilizando a biblioteca snake
    SnakeGame game;
    snake_init(&game);

    while (true) {
        // Atualiza o controle da cobrinha com base no joystick
        snake_update_direction(&game);
        // Atualiza a lógica do jogo (movimentação, alimentação, colisões e wrap-around)
        snake_update(&game);
        // Desenha o jogo no display OLED
        snake_draw(&game, &display);
        // Toca uma nota da melodia de fundo a cada iteração
        play_background_note();

        // Se ocorrer colisão (game over), toca o som de explosão, exibe a tela de Game Over e reinicia o jogo
        if (game.game_over_flag) {
            play_explosion_sound();
            snake_game_over_screen(&display);
            snake_init(&game);
        }
        
        sleep_ms(FRAME_DELAY);
    }
    
    return 0;
}

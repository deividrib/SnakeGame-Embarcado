#include "chapter1.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Definição de pinos e outros elementos necessários (mantidos)
#define LED_R_PIN       13
#define LED_G_PIN       11
#define LED_B_PIN       12
#define BUZZER_PIN      10
#define BUTTON_ACCEPT   5
#define BUTTON_DENY     6
#define MATRIX_OUT_PIN  7

// Glifo enigmático para a matriz LED 5x5
static double glifo[25] = {
    0.0, 0.3, 0.0, 0.3, 0.0,
    0.3, 0.0, 0.3, 0.0, 0.3,
    0.0, 0.3, 0.3, 0.3, 0.0,
    0.3, 0.0, 0.3, 0.0, 0.3,
    0.0, 0.3, 0.0, 0.3, 0.0
};

// Mantém o LED RGB azul
static void set_led_blue(void) {
    gpio_put(LED_R_PIN, 0);
    gpio_put(LED_G_PIN, 0);
    gpio_put(LED_B_PIN, 1);
}

// Toca uma única nota no buzzer
static void play_note(uint frequency, uint duration_ms) {
    if (frequency == 0) {
        sleep_ms(duration_ms);
        return;
    }
    
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel = pwm_gpio_to_channel(BUZZER_PIN);
    
    float divider = 100.0f;
    pwm_set_clkdiv(slice_num, divider);
    
    uint32_t wrap = (uint32_t)(125000000 / (divider * frequency)) - 1;
    pwm_set_wrap(slice_num, wrap);
    
    pwm_set_chan_level(slice_num, channel, wrap / 2);
    pwm_set_enabled(slice_num, true);
    
    sleep_ms(duration_ms);
    
    pwm_set_enabled(slice_num, false);
}

// Toca a musiquinha do jogo
static void play_tune(void) {
    play_note(262, 500);
    sleep_ms(50);
    play_note(294, 500);
    sleep_ms(50);
    play_note(330, 500);
    sleep_ms(50);
    play_note(262, 500);
}

// Exibe um frame no OLED
static void display_frame(ssd1306_t *display, const char *lines[], uint8_t y_positions[], int num_lines) {
    ssd1306_fill(display, 0);
    for (int i = 0; i < num_lines; i++) {
        ssd1306_draw_string(display, lines[i], 8, y_positions[i]);
    }
    ssd1306_send_data(display);
}

// Função principal do capítulo 1
void chapter1_run(ssd1306_t *display, pio_t *meu_pio) {
    // Exibe os frames de narrativa
    const char *frame1_lines[] = {
        "O Chamado do",
        "Destino",
        "Em uma noite",
        "calma,"
    };
    uint8_t frame1_y[] = {0, 10, 20, 30};
    display_frame(display, frame1_lines, frame1_y, 4);
    sleep_ms(7000);
    
    const char *frame2_lines[] = {
        "uma visao",
        "misteriosa",
        "revela um",
        "segredo antigo"
    };
    uint8_t frame2_y[] = {0, 10, 20, 30};
    display_frame(display, frame2_lines, frame2_y, 4);
    sleep_ms(7000);
    
    const char *frame3_lines[] = {
        "Aceita aventura?",
        "Aceitar: Botao 5",
        "Recusar: Botao 6"
    };
    uint8_t frame3_y[] = {10, 30, 40};
    display_frame(display, frame3_lines, frame3_y, 3);
    
    // Define a cor do glifo
    meu_pio->r = 1.0;
    meu_pio->g = 0.5;
    meu_pio->b = 0.0;
    desenho_pio_rgb(glifo, meu_pio);
    
    set_led_blue();
    play_tune();
    
    printf("Aguardando resposta...\n");
    while (gpio_get(BUTTON_ACCEPT) && gpio_get(BUTTON_DENY)) {
        tight_loop_contents();
    }
    
    if (!gpio_get(BUTTON_ACCEPT)) {
        ssd1306_fill(display, 0);
        ssd1306_draw_string(display, "Chamado Aceito!", 20, 20);
        ssd1306_draw_string(display, "Iniciando aventura...", 10, 40);
        ssd1306_send_data(display);
        printf("Chamado Aceito!\n");
    } else if (!gpio_get(BUTTON_DENY)) {
        ssd1306_fill(display, 0);
        ssd1306_draw_string(display, "Chamado Recusado!", 20, 20);
        ssd1306_draw_string(display, "Fim da jornada.", 20, 40);
        ssd1306_send_data(display);
        printf("Chamado Recusado!\n");
    }
    
    while (1) {
        tight_loop_contents();
    }
}

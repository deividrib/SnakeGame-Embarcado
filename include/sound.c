#include "sound.h"
#include "pico/stdlib.h"

// Função para tocar um tom utilizando PWM no pino especificado
void play_tone(uint gpio, uint32_t frequency, uint32_t duration_ms) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint channel = pwm_gpio_to_channel(gpio);
    uint32_t clk = clock_get_hz(clk_sys);
    uint32_t wrap = clk / frequency - 1;
    pwm_set_wrap(slice_num, wrap);
    // Define duty cycle de 50%
    pwm_set_chan_level(slice_num, channel, wrap / 2);
    pwm_set_enabled(slice_num, true);
    sleep_ms(duration_ms);
    pwm_set_enabled(slice_num, false);
}

// Definindo uma melodia de fundo mais animada com notas (frequência e duração)
static note_t background_melody[] = {
    {523, 100},  // C5
    {587, 100},  // D5
    {659, 100},  // E5
    {698, 100},  // F5
    {784, 100},  // G5
    {880, 100},  // A5
    {988, 100},  // B5
    {1047,200},  // C6 (nota prolongada)
    {988, 100},
    {880, 100},
    {784, 100},
    {698, 100},
    {659, 100},
    {587, 100},
    {523, 200}   // Finalizando com C5 prolongado
};
static size_t background_melody_length = sizeof(background_melody) / sizeof(background_melody[0]);
static size_t current_note_index = 0;

// Função que toca a próxima nota da melodia de fundo no buzzer de fundo
void sound_play_background_note(void) {
    note_t note = background_melody[current_note_index];
    play_tone(BUZZER_BG, note.frequency, note.duration_ms);
    current_note_index = (current_note_index + 1) % background_melody_length;
}

// Função que reproduz um som de explosão, utilizando uma sequência de tons descendentes
void sound_play_explosion_sound(void) {
    uint32_t explosion[] = {600, 500, 400, 300, 200};
    size_t explosion_length = sizeof(explosion) / sizeof(explosion[0]);
    for (size_t i = 0; i < explosion_length; i++) {
        play_tone(BUZZER_EXP, explosion[i], 50);
    }
}

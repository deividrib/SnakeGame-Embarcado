#include "sound.h"
#include "pico/stdlib.h"

// Função para tocar um tom utilizando PWM no pino especificado.
// Após desabilitar o PWM, o pino é redefinido como saída digital em nível baixo,
// evitando aquecimento e possíveis ruídos residuais.
void play_tone(uint gpio, uint32_t frequency, uint32_t duration_ms) {
    if (frequency == 0) {
        sleep_ms(duration_ms);
        return;
    }
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    uint channel = pwm_gpio_to_channel(gpio);
    uint32_t clk = clock_get_hz(clk_sys);
    uint32_t wrap = clk / frequency - 1;
    pwm_set_wrap(slice_num, wrap);
    // Duty cycle de 50%
    pwm_set_chan_level(slice_num, channel, wrap / 2);
    pwm_set_enabled(slice_num, true);
    sleep_ms(duration_ms);
    pwm_set_enabled(slice_num, false);

    // Redefine o pino para função digital e força nível baixo
    gpio_set_function(gpio, GPIO_FUNC_SIO);
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
    gpio_put(gpio, 0);
}

// Melodia de fundo "snake game" – uma sequência simples, rítmica e retro,
// que evoca o movimento sinuoso da cobrinha e cria um ambiente divertido.
static note_t background_melody[] = {
    {261, 100}, // C4: inicia a escala com um tom suave
    {293, 100}, // D4
    {329, 100}, // E4
    {349, 100}, // F4
    {392, 100}, // G4: ponto alto que sugere dinamismo
    {392, 100}, // G4: repetição para enfatizar o movimento
    {349, 100}, // F4: descendo de forma sutil
    {329, 100}, // E4
    {293, 100}, // D4
    {261, 150}, // C4: nota de resolução com duração maior
    {0,   50}   // Pausa para separar as repetições da melodia
};

static size_t background_melody_length = sizeof(background_melody) / sizeof(background_melody[0]);
static size_t current_note_index = 0;

// Função que toca a próxima nota da melodia de fundo no buzzer designado para o jogo
void sound_play_background_note(void) {
    note_t note = background_melody[current_note_index];
    play_tone(BUZZER_BG, note.frequency, note.duration_ms);
    current_note_index = (current_note_index + 1) % background_melody_length;
}

// Função que reproduz um som de explosão com uma sequência de tons descendentes,
// ideal para eventos como colisões ou fim de jogo.
void sound_play_explosion_sound(void) {
    uint32_t explosion[] = {600, 550, 500, 450, 400, 350, 300, 250, 200};
    size_t explosion_length = sizeof(explosion) / sizeof(explosion[0]);
    for (size_t i = 0; i < explosion_length; i++) {
        // A duração aumenta à medida que a frequência decai, enfatizando a queda do tom
        uint32_t duration = 30 + i * 5;
        play_tone(BUZZER_EXP, explosion[i], duration);
        sleep_ms(10); // Breve pausa para efeito dramático
    }
}

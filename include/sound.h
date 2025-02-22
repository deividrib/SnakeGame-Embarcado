#ifndef SOUND_H
#define SOUND_H

#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#ifdef __cplusplus
extern "C" {
#endif

// Define os pinos para os buzzers, se ainda não estiverem definidos
#ifndef BUZZER_BG
#define BUZZER_BG 10   // Buzzer para música de fundo
#endif

#ifndef BUZZER_EXP
#define BUZZER_EXP 21  // Buzzer para som de explosão
#endif

// Estrutura para representar uma nota (frequência e duração)
typedef struct {
    uint32_t frequency;
    uint32_t duration_ms;
} note_t;

// Protótipos das funções da biblioteca de som
void play_tone(uint gpio, uint32_t frequency, uint32_t duration_ms);
void sound_play_background_note(void);
void sound_play_explosion_sound(void);

#ifdef __cplusplus
}
#endif

#endif // SOUND_H

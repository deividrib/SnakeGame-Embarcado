#ifndef HIGHSCORE_H
#define HIGHSCORE_H

#include "ssd1306.h"  // Certifique-se de incluir o cabeçalho do OLED

#define MAX_HIGH_SCORES 3
#define MAX_NAME_LENGTH 16

typedef struct {
    int score;
    char name[MAX_NAME_LENGTH];
} HighScore;

// Inicializa o placar com valores padrão.
void init_high_scores();

// Atualiza o placar se a pontuação for um novo recorde e solicita o nome do jogador.
void update_high_scores(ssd1306_t *display, int score);

// Exibe o placar na tela OLED.
void display_scoreboard(ssd1306_t *display);

#endif // HIGHSCORE_H

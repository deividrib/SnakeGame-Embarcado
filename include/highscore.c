#include "highscore.h"
#include <stdio.h>
#include <string.h>

HighScore high_scores[MAX_HIGH_SCORES];

void init_high_scores() {
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        high_scores[i].score = 0;
        strcpy(high_scores[i].name, "---");
    }
}

void update_high_scores(ssd1306_t *display, int score) {
    if (score > high_scores[MAX_HIGH_SCORES - 1].score) {
        char name[9] = {0};

        // Exibe mensagem no OLED para entrada do nome
        ssd1306_fill(display, 0);
        ssd1306_draw_string(display, "Novo recorde!", 10, 10);
        ssd1306_draw_string(display, "Dgt seu nome:", 10, 25);
        ssd1306_draw_string(display, "via Serial", 10, 40);
        ssd1306_draw_string(display, "Aguarde 8s", 10, 55);
        ssd1306_send_data(display);

        // Solicita o nome via Serial
        printf("Novo recorde! Insira seu nome (max 8 caracteres):\n");
        fflush(stdout);

        int index = 0;
        int elapsed = 0;
        // Aguarda até 8s para a entrada do usuário (verifica a cada 10ms)
        while (elapsed < 8000) {
            int ch = getchar_timeout_us(10000); // timeout de 10 ms
            if (ch != PICO_ERROR_TIMEOUT) {
                if (ch == '\n' || ch == '\r') {
                    break;
                }
                if (index < 8) {
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

        printf("Nome registrado: %s - %d pontos\n", name, score);
        fflush(stdout);
    }
}

void display_scoreboard(ssd1306_t *display) {
    char buffer[32];
    ssd1306_fill(display, 0);
    ssd1306_draw_string(display, "Placar", 30, 0);
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        snprintf(buffer, sizeof(buffer), "%d. %s - %d", i + 1, high_scores[i].name, high_scores[i].score);
        ssd1306_draw_string(display, buffer, 0, 10 + i * 10);
    }
    ssd1306_draw_string(display, "Aperte BTN ", 0, 50);
    ssd1306_send_data(display);
}

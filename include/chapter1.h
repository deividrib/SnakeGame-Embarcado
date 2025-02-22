#ifndef CHAPTER1_H
#define CHAPTER1_H

#include "ssd1306.h"
#include "matriz_led_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// Função para executar o Capítulo 1
void chapter1_run(ssd1306_t *display, pio_t *meu_pio);

#ifdef __cplusplus
}
#endif

#endif // CHAPTER1_H

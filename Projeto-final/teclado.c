#include "teclado.h"

// linhas
uint8_t rowPins[ROWSPIN] = {4, 8, 9, 16};

// colunas
uint8_t colPins[COLSPIN] = {17, 18, 19, 20};

// mapa das teclas
char keyMap[ROWSPIN][COLSPIN] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

// ////////////////

void teclado_init(void) {
    // linhas como saída em HIGH
    for (int i = 0; i < ROWSPIN; i++) {
        gpio_init(rowPins[i]);
        gpio_set_dir(rowPins[i], GPIO_OUT);
        gpio_put(rowPins[i], 1);
    }

    // colunas como entrada com pull-up
    for (int i = 0; i < COLSPIN; i++) {
        gpio_init(colPins[i]);
        gpio_set_dir(colPins[i], GPIO_IN);
        gpio_pull_up(colPins[i]);
    }
}

char teclado_read(void) {
    static char last_key = 0;

    for (int l = 0; l < ROWSPIN; l++) {

        // ativa linha atual
        gpio_put(rowPins[l], 0);
        sleep_us(5);

        for (int c = 0; c < COLSPIN; c++) {
            if (gpio_get(colPins[c]) == 0) {

                char key = keyMap[l][c];

                // desativa linha antes de retornar
                gpio_put(rowPins[l], 1);

                // só retorna se for uma nova tecla
                if (key != last_key) {
                    last_key = key;
                    sleep_ms(120); // debounce
                    return key;
                }
            }
        }

        // desativa linha
        gpio_put(rowPins[l], 1);
    }

    // nenhuma tecla pressionada libera próxima leitura
    last_key = 0;
    return 0;
}
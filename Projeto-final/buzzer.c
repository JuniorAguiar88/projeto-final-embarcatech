#include "buzzer.h"
#include "pico/stdlib.h"
#include "hardware/pwm.h"

#define BUZZER_PIN 21   // Buzzer principal

void buzzer_init(void) {
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
}

void tocarBuzzer(uint32_t frequencia) {
    if (frequencia == 0) {
        gpio_put(BUZZER_PIN, 0);
        return;
    }

    // Configura o PWM para gerar o tom
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel = pwm_gpio_to_channel(BUZZER_PIN);
    pwm_set_clkdiv(slice_num, 125.0f);
    pwm_set_wrap(slice_num, 1000000 / frequencia);
    pwm_set_chan_level(slice_num, channel, 500000 / frequencia);
    pwm_set_enabled(slice_num, true);
}

void pararBuzzer(void) {
    gpio_put(BUZZER_PIN, 0);
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
}

void beepCofre(void) {
    // Beep curto e limpo como de um teclado de cofre eletrônico
    tocarBuzzer(800);
    sleep_ms(50);
    pararBuzzer();
}

void beepCofreErro(void) {
    // Beep de erro - dois bips graves
    tocarBuzzer(400);
    sleep_ms(200);
    pararBuzzer();
    sleep_ms(100);
    tocarBuzzer(400);
    sleep_ms(200);
    pararBuzzer();
}

// ✨ Nova função: Melodia de vitória ao acertar a senha
void beepVitoria(void) {
    // Sequência alegre ascendente (dó-ré-mi-fá-sol da oitava 5)
    const uint16_t notas[] = {523, 587, 659, 698, 784}; // C5, D5, E5, F5, G5
    const uint8_t duracoes[] = {100, 100, 100, 100, 150};
    const size_t num_notas = sizeof(notas) / sizeof(notas[0]);
    
    for (size_t i = 0; i < num_notas; i++) {
        tocarBuzzer(notas[i]);
        sleep_ms(duracoes[i]);
        pararBuzzer();
        if (i < num_notas - 1) {
            sleep_ms(30); // Pequena pausa entre notas para clareza
        }
    }
    
    // Pausa final para completar o efeito
    sleep_ms(50);
}
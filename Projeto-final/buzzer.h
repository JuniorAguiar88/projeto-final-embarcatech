#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

// Inicializa o buzzer
void buzzer_init(void);

// Toca um tom com a frequência especificada
void tocarBuzzer(uint32_t frequencia);

// Para o buzzer
void pararBuzzer(void);

// Beeps específicos
void beepCofre(void);
void beepCofreErro(void);  // Nova função
void beepVitoria(void);    //  Nova função de vitória

#endif // BUZZER_H
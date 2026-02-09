#ifndef SERVO_MOTOR_H
#define SERVO_MOTOR_H

#include <stdio.h>
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"

// --- Constantes para o controle do Servo ---
#define PWM_PERIOD_MS 20
#define SERVO_PIN 2  // GPIO 2 para controle do servo
#define SERVO_MIN_PULSE_US 500
#define SERVO_MAX_PULSE_US 2500

// Function prototypes
void servo_init(void);
void servo_set_angle(uint angle);

#endif // SERVO_MOTOR_H
#ifndef SERVO_H
#define SERVO_H

#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <stdint.h>

/* ===== Configurações do Servo ===== */
#define PWM_FREQ        50
#define TOP             25000
#define SERVO_PIN       28
#define CLOCK_DIVIDER   100.0f

/* ===== Funções ===== */

/* Variáveis globais do PWM */
static uint slice_num;
static uint channel;

/* Converte ângulo (0–180°) em pulso (us) */
uint16_t calcula_pulso(uint16_t angulo)
{
    if (angulo > 180) angulo = 180;
    return 500 + (angulo * (2400 - 500) / 180);
}

/* Define posição do servo pelo pulso */
static void posicao(uint16_t pulse_us)
{
    uint16_t level = (pulse_us * TOP) / 20000;
    pwm_set_chan_level(slice_num, channel, level);
}

/* Função pública: define ângulo do servo */
void servo_set_angulo(uint16_t angulo)
{
    uint16_t pulse = calcula_pulso(angulo);
    posicao(pulse);
}

/* Inicialização do PWM do servo */
void servo_config(void)
{
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);

    slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    channel   = pwm_gpio_to_channel(SERVO_PIN);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, CLOCK_DIVIDER);
    pwm_config_set_wrap(&config, TOP);

    pwm_init(slice_num, &config, true);
}


#endif

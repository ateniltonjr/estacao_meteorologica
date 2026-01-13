#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"
//#include "cyw43_arch.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "font.h"
#include "matrixws.h"
#include "leds_buttons.h"
#include "display.h"
#include "interface.h"
#include "pagina.h"
#include "servo.h"

/* ================= CONFIGURAÇÕES ================= */
#define WIFI_SSID "Hill"
#define WIFI_PASS "Terco8485"

#define debounce_delay 300

#define POT1_GPIO 26   // ADC0
#define POT2_GPIO 27   // ADC1
#define POT1_ADC  0
#define POT2_ADC  1
/* ================================================= */

volatile uint tempo_interrupcao = 0;

/* ================= VARIÁVEIS ADC ================= */
uint16_t adc_raw_pot1;
uint16_t adc_raw_pot2;
float pot1_percent;
float pot2_percent;
/* ================================================= */

/* ================= SENSORES ================= */
AHT20_Data data;
int32_t raw_temp_bmp;
int32_t raw_pressure;
struct bmp280_calib_param params;
/* =============================================== */

/* ================= INTERRUPÇÕES ================= */
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint tempo_atual = to_ms_since_boot(get_absolute_time());
    if (tempo_atual - tempo_interrupcao > debounce_delay)
    {
        if (gpio == BOTAO_A)
        {
            pagina_atual = (pagina_atual + 1) % 5;
            printf("Botão A pressionado! Página: %d\n", pagina_atual);
        }
        else if (gpio == BOTAO_B)
        {
            reset_usb_boot(0, 0);
        }
        tempo_interrupcao = tempo_atual;
    }
}
/* ================================================= */

/* ================= WIFI ================= */
int iniciar_wifi(void)
{
    if (cyw43_arch_init())
        return 1;

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID, WIFI_PASS,
            CYW43_AUTH_WPA2_AES_PSK, 10000))
        return 1;

    return 0;
}
/* ======================================= */

/* ================= I2C ================= */
void iniciar_I2Cs(void)
{
    i2c_init(i2c0, 400 * 1000);
    gpio_set_function(0, GPIO_FUNC_I2C);
    gpio_set_function(1, GPIO_FUNC_I2C);
    gpio_pull_up(0);
    gpio_pull_up(1);
}
/* ======================================= */

/* ================= SENSORES ================= */
void dados_sensores(void)
{
    bmp280_init(i2c0);
    bmp280_get_calib_params(i2c0, &params);

    aht20_reset(i2c0);
    aht20_init(i2c0);
}
/* ============================================ */

/* ================= ADC ================= */
void ler_potenciometros(void)
{
    adc_select_input(POT1_ADC);
    adc_raw_pot1 = adc_read();
    pot1_percent = (adc_raw_pot1 / 4095.0f) * 100.0f;

    adc_select_input(POT2_ADC);
    adc_raw_pot2 = adc_read();
    pot2_percent = (adc_raw_pot2 / 4095.0f) * 100.0f;
}
/* ====================================== */

/* ================= SERVO ================= */
/* Vem do pagina.h */
extern float servo_percent;
/* ======================================== */

int main(void)
{
    stdio_init_all();
    sleep_ms(1500);

    inicializar_leds();
    iniciar_buzzer();
    controle(PINO_MATRIZ);
    iniciar_botoes();
    init_display();
    iniciar_I2Cs();

    /* ===== SERVO ===== */
    servo_config();

    /* ===== ADC ===== */
    adc_init();
    adc_gpio_init(POT1_GPIO);
    adc_gpio_init(POT2_GPIO);

    /* ===== WIFI ===== */
    iniciar_wifi();
    start_http_server();
    dados_sensores();

    printf("Sistema iniciado\n");

    /* ================= LOOP PRINCIPAL ================= */
    while (true)
    {
        /* -------- ADC -------- */
        ler_potenciometros();

        /* -------- SERVO VIA ADC -------- */
        uint16_t angulo = (uint16_t)((pot1_percent / 100.0f) * 180.0f);
        servo_set_angulo(angulo);

        printf("POT1: %.1f %% | Angulo servo: %u°\n",
               pot1_percent, angulo);

        /* -------- WIFI -------- */
        cyw43_arch_poll();

        /* -------- BMP280 -------- */
        bmp280_read_raw(i2c0, &raw_temp_bmp, &raw_pressure);
        int32_t temperature = bmp280_convert_temp(raw_temp_bmp, &params);
        int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temp_bmp, &params);
        double altitude = calculate_altitude(pressure);

        /* -------- AHT20 -------- */
        int aht_ok = aht20_read(i2c0, &data);

        /* -------- HISTÓRICO -------- */
        float valores[5];
        valores[0] = (pressure / 1000.0f) + offsets[0];
        valores[1] = (temperature / 100.0f) + offsets[1];
        valores[2] = altitude + offsets[2];
        valores[3] = aht_ok ? data.temperature + offsets[3] : valores[3];
        valores[4] = aht_ok ? data.humidity + offsets[4] : valores[4];

        for (int i = 0; i < 5; i++)
        {
            historico[i][historico_idx[i]] = valores[i];
            historico_idx[i] =
                (historico_idx[i] + 1) % HIST_SIZE;
        }

        /* -------- DISPLAY -------- */
        ssd1306_fill(&ssd, false);
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", valores[pagina_atual]);
        escrever(&ssd, nomes_leituras[pagina_atual], 5, 10, cor);
        escrever(&ssd, buf, 5, 30, cor);
        ssd1306_send_data(&ssd);

        sleep_ms(50);
    }
}


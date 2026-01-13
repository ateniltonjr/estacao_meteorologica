#ifndef PAGINA_H
#define PAGINA_H 

#include "aht20.h"
#include "bmp280.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "interface.h"

/* ================= CONTROLE DE PÁGINA ================= */

volatile uint8_t pagina_atual = 0;

const char* nomes_leituras[5] = {
    "Pressão",
    "Temperatura BMP280",
    "Altitude",
    "Temperatura AHT20",
    "Umidade"
};

#define SEA_LEVEL_PRESSURE 101325.0

/* ================= HISTÓRICO E CONFIG ================= */

#define HIST_SIZE 30

float historico[5][HIST_SIZE] = {{0}};
uint8_t historico_idx[5] = {0};

float limites_min[5] = {0, 0, 0, 0, 0};
float limites_max[5] = {1000, 100, 10000, 50, 100};
float offsets[5]     = {0, 0, 0, 0, 0};

/* ================= POTENCIÔMETROS ================= */
/* Vêm do main.c */

extern float pot1_percent;
extern float pot2_percent;

/* ================= FUNÇÕES ================= */

double calculate_altitude(double pressure)
{
    return 44330.0 * (1.0 - pow(pressure / SEA_LEVEL_PRESSURE, 0.1903));
}

/* ================= HTTP ================= */

struct http_state
{
    char response[4096];
    size_t len;
    size_t sent;
};

static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    struct http_state *hs = (struct http_state *)arg;
    hs->sent += len;
    if (hs->sent >= hs->len)
    {
        tcp_close(tpcb);
        free(hs);
    }
    return ERR_OK;
}

static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char req[1024] = {0};
    pbuf_copy_partial(p, req, sizeof(req) - 1, 0);

    struct http_state *hs = malloc(sizeof(struct http_state));
    if (!hs)
    {
        pbuf_free(p);
        tcp_close(tpcb);
        return ERR_MEM;
    }

    /* ================= POST /config ================= */
    if (strstr(req, "POST /config"))
    {
        int pagina = -1;
        float lim_min = 0, lim_max = 0, offset = 0;

        char *json = strchr(req, '{');
        if (json)
        {
            sscanf(json,
                "{\"pagina\":%d,\"lim_min\":%f,\"lim_max\":%f,\"offset\":%f",
                &pagina, &lim_min, &lim_max, &offset);

            if (pagina >= 0 && pagina < 5)
            {
                limites_min[pagina] = lim_min;
                limites_max[pagina] = lim_max;
                offsets[pagina]     = offset;
            }
        }

        int len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\nContent-Length:2\r\n\r\nok");

        tcp_write(tpcb, hs->response, len, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
    }

    /* ================= POST /pagina ================= */
    else if (strstr(req, "POST /pagina"))
    {
        pagina_atual = (pagina_atual + 1) % 5;

        int len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\nContent-Length:2\r\n\r\nok");

        tcp_write(tpcb, hs->response, len, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
    }

    /* ================= GET /estado ================= */
    else if (strstr(req, "GET /estado"))
    {
        char historico_str[HIST_SIZE * 12] = "";
        int idx = historico_idx[pagina_atual];
        int n = 0;

        for (int i = 0; i < HIST_SIZE; i++)
        {
            int pos = (idx + i) % HIST_SIZE;
            n += snprintf(
                historico_str + n,
                sizeof(historico_str) - n,
                (i == 0 ? "[%.2f" : ",%.2f"),
                historico[pagina_atual][pos]
            );
        }
        snprintf(historico_str + n, sizeof(historico_str) - n, "]");

        const char* unidade = "";
        switch (pagina_atual)
        {
            case 0: unidade = "kPa"; break;
            case 1: unidade = "°C";  break;
            case 2: unidade = "m";   break;
            case 3: unidade = "°C";  break;
            case 4: unidade = "%";   break;
        }

        float valor_atual =
            historico[pagina_atual][(historico_idx[pagina_atual] + HIST_SIZE - 1) % HIST_SIZE];

        char json[1400];
        int json_len = snprintf(json, sizeof(json),
            "{"
            "\"pagina\":%d,"
            "\"nome\":\"%s\","
            "\"valor\":%.2f,"
            "\"unidade\":\"%s\","
            "\"historico\":%s,"
            "\"lim_min\":%.2f,"
            "\"lim_max\":%.2f,"
            "\"offset\":%.2f,"
            "\"pot1\":%.1f,"
            "\"pot2\":%.1f"
            "}",
            pagina_atual,
            nomes_leituras[pagina_atual],
            valor_atual,
            unidade,
            historico_str,
            limites_min[pagina_atual],
            limites_max[pagina_atual],
            offsets[pagina_atual],
            pot1_percent,
            pot2_percent
        );

        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n%s",
            json_len, json
        );

        tcp_write(tpcb, hs->response, hs->len, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
    }

    /* ================= HTML ================= */
    else
    {
        hs->len = snprintf(hs->response, sizeof(hs->response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %d\r\n\r\n%s",
            (int)strlen(HTML_BODY), HTML_BODY);

        tcp_write(tpcb, hs->response, hs->len, TCP_WRITE_FLAG_COPY);
        tcp_output(tpcb);
    }

    pbuf_free(p);
    free(hs);
    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}

static void start_http_server(void)
{
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) return;

    tcp_bind(pcb, IP_ADDR_ANY, 80);
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);

    printf("Servidor HTTP ativo na porta 80\n");
}

#endif

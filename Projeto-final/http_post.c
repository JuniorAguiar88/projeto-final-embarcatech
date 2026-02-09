// http_post.c
#include "http_post.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include <stdio.h>
#include <string.h>

static ip_addr_t server_ip;
static bool ip_resolved = false;

// Estrutura para passar dados ao callback TCP
typedef struct {
    float temperatura;
    float umidade;
    const char* status;
} HttpData;

// Callback DNS - chamado quando IP é resolvido
static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr) {
        server_ip = *ipaddr;
        ip_resolved = true;
        printf("[HTTP] IP resolvido: %s\n", ipaddr_ntoa(&server_ip));
    } else {
        printf("[HTTP] ERRO: Falha ao resolver DNS\n");
        ip_resolved = false;
    }
}

// Callback TCP - envia requisição HTTP após conexão
static err_t tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err != ERR_OK || tpcb == NULL) {
        printf("[HTTP] ERRO na conexão TCP: %d\n", err);
        if (tpcb) tcp_close(tpcb);
        return ERR_OK;
    }

    HttpData *data = (HttpData*)arg;
    
    // Monta payload JSON
    char payload[200];
    snprintf(payload, sizeof(payload), 
        "{\"temperatura\":%.1f,\"umidade\":%.1f,\"status\":\"%s\"}",
        data->temperatura, data->umidade, data->status);

    // Monta requisição HTTP
    char request[500];
    snprintf(request, sizeof(request),
        "POST /cofre-data HTTP/1.1\r\n"
        "Host: %s:1880\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        HTTP_SERVER_IP, strlen(payload), payload);

    // Envia requisição
    cyw43_arch_lwip_begin();
    tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    cyw43_arch_lwip_end();

    printf("[HTTP] Dados enviados: Temp=%.1f°C Umid=%.1f%% Status=%s\n", 
           data->temperatura, data->umidade, data->status);
    
    // Fecha conexão após envio
    tcp_close(tpcb);
    return ERR_OK;
}

void http_init(void) {
    printf("[HTTP] Inicializando cliente HTTP...\n");
    ip_resolved = false;
}

bool http_post_json(float temperatura, float umidade, const char* status) {
    // Resolve DNS na primeira vez
    if (!ip_resolved) {
        printf("[HTTP] Resolvendo DNS: %s\n", HTTP_SERVER_IP);
        cyw43_arch_lwip_begin();
        err_t dns_err = dns_gethostbyname(HTTP_SERVER_IP, &server_ip, dns_callback, NULL);
        cyw43_arch_lwip_end();
        
        if (dns_err != ERR_OK && dns_err != ERR_INPROGRESS) {
            printf("[HTTP] ERRO DNS imediato: %d\n", dns_err);
            return false;
        }
        
        // Aguarda resolução (máx 2 segundos)
        uint32_t timeout = 2000;
        while (!ip_resolved && timeout > 0) {
            sleep_ms(10);
            cyw43_arch_poll(); // Mantém WiFi ativo durante espera
            timeout -= 10;
        }
        
        if (!ip_resolved) {
            printf("[HTTP] Timeout na resolução DNS\n");
            return false;
        }
    }

    // Cria PCB TCP
    struct tcp_pcb *pcb;
    cyw43_arch_lwip_begin();
    pcb = tcp_new();
    if (!pcb) {
        cyw43_arch_lwip_end();
        printf("[HTTP] ERRO: Falha ao criar PCB TCP\n");
        return false;
    }

    // Prepara dados para callback
    static HttpData data;
    data.temperatura = temperatura;
    data.umidade = umidade;
    data.status = status;

    // Conecta ao servidor (porta 1880 do Node-RED)
    err_t err = tcp_connect(pcb, &server_ip, 1880, tcp_connected_callback);
    if (err != ERR_OK) {
        tcp_close(pcb);
        cyw43_arch_lwip_end();
        printf("[HTTP] ERRO tcp_connect: %d\n", err);
        return false;
    }
    
    // Associa dados ao PCB para uso no callback
    tcp_arg(pcb, &data);
    cyw43_arch_lwip_end();

    return true;
}
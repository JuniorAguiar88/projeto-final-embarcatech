#include "http_post.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/dns.h"
#include "lwip/ip_addr.h"
#include <stdio.h>
#include <string.h>

static ip_addr_t server_ip;
static bool ip_ready = false;

// Armazenamento temporário seguro para dados do callback
static float temp_buffer = 0.0f;
static float umid_buffer = 0.0f;
static const char* status_buffer = NULL;

// Callback DNS (usado apenas para hostnames, não para IPs)
static void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
    if (ipaddr) {
        server_ip = *ipaddr;
        ip_ready = true;
        printf("[HTTP] ✅ DNS resolvido: %s -> %s\n", name, ipaddr_ntoa(&server_ip));
    } else {
        printf("[HTTP] ❌ Falha na resolução DNS para: %s\n", name);
        ip_ready = false;
    }
}

// Verifica se é um IP válido (formato xxx.xxx.xxx.xxx)
static bool is_ip_address(const char *str) {
    int nums[4];
    return sscanf(str, "%d.%d.%d.%d", &nums[0], &nums[1], &nums[2], &nums[3]) == 4 &&
           nums[0] >= 0 && nums[0] <= 255 &&
           nums[1] >= 0 && nums[1] <= 255 &&
           nums[2] >= 0 && nums[2] <= 255 &&
           nums[3] >= 0 && nums[3] <= 255;
}

// Callback TCP - envia requisição HTTP
static err_t tcp_connected_callback(void *arg, struct tcp_pcb *tpcb, err_t err) {
    if (err != ERR_OK || tpcb == NULL) {
        if (tpcb) tcp_close(tpcb);
        printf("[HTTP] ❌ Erro TCP: %d\n", err);
        return ERR_OK;
    }

    // Usa buffers globais seguros (sem conversões ilegais)
    char payload[100];
    snprintf(payload, sizeof(payload), 
        "{\"temperatura\":%.1f,\"umidade\":%.1f,\"status\":\"%s\"}",
        temp_buffer, umid_buffer, status_buffer ? status_buffer : "DESCONHECIDO");

    // Monta requisição HTTP completa
    char request[350];
    snprintf(request, sizeof(request),
        "POST /cofre-data HTTP/1.1\r\n"
        "Host: %s:1880\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        HTTP_SERVER_IP, (int)strlen(payload), payload);

    // Envia requisição
    cyw43_arch_lwip_begin();
    tcp_write(tpcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    cyw43_arch_lwip_end();

    printf("[HTTP] ✅ Enviado: Temp=%.1f°C Umid=%.1f%% Status=%s\n", 
           temp_buffer, umid_buffer, status_buffer ? status_buffer : "DESCONHECIDO");
    
    // Fecha conexão
    tcp_close(tpcb);
    return ERR_OK;
}

void http_init(void) {
    printf("[HTTP] Inicializando cliente HTTP...\n");
    
    // ✅ CORREÇÃO: Detecta se é IP ou hostname
    if (is_ip_address(HTTP_SERVER_IP)) {
        // Converte string IP diretamente para estrutura lwIP (SEM DNS!)
        if (ipaddr_aton(HTTP_SERVER_IP, &server_ip)) {
            ip_ready = true;
            printf("[HTTP] ✅ IP direto configurado: %s\n", HTTP_SERVER_IP);
        } else {
            printf("[HTTP] ❌ IP inválido: %s\n", HTTP_SERVER_IP);
            ip_ready = false;
        }
    } else {
        // Usa DNS apenas para hostnames (ex: "meupc.local")
        printf("[HTTP] Resolvendo hostname via DNS: %s\n", HTTP_SERVER_IP);
        ip_ready = false;
        cyw43_arch_lwip_begin();
        dns_gethostbyname(HTTP_SERVER_IP, &server_ip, dns_callback, NULL);
        cyw43_arch_lwip_end();
        
        // Aguarda resolução (máx 2s)
        uint32_t timeout = 2000;
        while (!ip_ready && timeout > 0) {
            sleep_ms(10);
            cyw43_arch_poll();
            timeout -= 10;
        }
        if (!ip_ready) {
            printf("[HTTP] ⚠️ Timeout DNS para hostname\n");
        }
    }
}

bool http_post_json(float temperatura, float umidade, const char* status) {
    if (!ip_ready) {
        printf("[HTTP] ❌ IP/hostname não resolvido\n");
        return false;
    }

    // Armazena dados em buffers globais seguros
    temp_buffer = temperatura;
    umid_buffer = umidade;
    status_buffer = status;

    // Cria PCB TCP
    struct tcp_pcb *pcb;
    cyw43_arch_lwip_begin();
    pcb = tcp_new();
    if (!pcb) {
        cyw43_arch_lwip_end();
        printf("[HTTP] ❌ Falha ao criar PCB TCP\n");
        return false;
    }

    // Conecta ao servidor
    err_t err = tcp_connect(pcb, &server_ip, 1880, tcp_connected_callback);
    if (err != ERR_OK) {
        tcp_close(pcb);
        cyw43_arch_lwip_end();
        printf("[HTTP] ❌ tcp_connect erro: %d\n", err);
        return false;
    }
    
    cyw43_arch_lwip_end();
    return true;
}

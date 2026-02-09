#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"  // WiFi Pico W - DEVE SER INCLUÍDO PRIMEIRO
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "matriz_leds.h"
#include "teclado.h"
#include "buzzer.h"
#include "display.h"
#include "servo_motor.h"
#include "aht10.h"
#include "http_post.h"  // HTTP para dashboard
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define LED_PIN 13      // Led vermelho (GPIO 13 - seguro)

// --- Constantes para o Servo ---
#define SERVO_FECHADO 0
#define SERVO_ABERTO 90

// Definições de login/senha
#define LOGIN_CORRETO "8450"
#define SENHA_CORRETA "9261"
#define MAX_DIGITOS 4

// Estados do sistema
typedef enum {
    ESTADO_AGUARDANDO_LOGIN,
    ESTADO_VERIFICANDO_LOGIN,
    ESTADO_LOGIN_INVALIDO,
    ESTADO_AGUARDANDO_SENHA,
    ESTADO_VERIFICANDO_SENHA,
    ESTADO_SENHA_INVALIDA,
    ESTADO_ACESSO_CONCEDIDO,
    ESTADO_ERRO
} EstadoSistema;

// Buffer para armazenar entrada
char entrada_atual[MAX_DIGITOS + 1] = "";
int entrada_count = 0;
EstadoSistema estado_atual = ESTADO_AGUARDANDO_LOGIN;
char login_usuario[MAX_DIGITOS + 1] = "";

// Dados do sensor
float temperatura = 0.0f;
float umidade = 0.0f;
absolute_time_t ultimo_tempo_leitura = {0};
const uint32_t INTERVALO_LEITURA_MS = 2000;

// Controle de transições automáticas
absolute_time_t tempo_transicao = {0};
bool aguardando_transicao = false;
EstadoSistema estado_destino = ESTADO_AGUARDANDO_LOGIN;

// Flag para beep de vitória
bool tocar_beep_vitoria = false;

// Flags para controle do servo
bool servo_aberto = false;

// Símbolos para a matriz de LED (V e X)
const Matriz_leds_config simbolo_v = {
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},
    {{0.0, 0.5, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.5, 0.0, 0.0}},
    {{0.0, 0.0, 0.0}, {0.0, 0.5, 0.0}, {0.0, 0.0, 0.0}, {0.5, 0.0, 0.0}, {0.0, 0.0, 0.0}},
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.5, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.5, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}
};

const Matriz_leds_config simbolo_x = {
    {{0.5, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.5, 0.0, 0.0}},
    {{0.0, 0.0, 0.0}, {0.5, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.5, 0.0, 0.0}, {0.0, 0.0, 0.0}},
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.5, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},
    {{0.0, 0.0, 0.0}, {0.5, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.5, 0.0, 0.0}, {0.0, 0.0, 0.0}},
    {{0.5, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.5, 0.0, 0.0}}
};

const Matriz_leds_config interrogacao = {
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.5}, {0.0, 0.0, 0.5}, {0.0, 0.0, 0.5}, {0.0, 0.0, 0.0}},
    {{0.0, 0.0, 0.5}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.5}},
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.5}, {0.0, 0.0, 0.0}},
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.5}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}},
    {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.5}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}}
};

// Função auxiliar
void blink_led(void) {
    gpio_put(LED_PIN, 1);
    sleep_ms(100);
    gpio_put(LED_PIN, 0);
    sleep_ms(100);
}

// Função para limpar entrada
void limpar_entrada(void) {
    entrada_count = 0;
    entrada_atual[0] = '\0';
}

// Função para adicionar dígito à entrada
void adicionar_digito(char digito) {
    if (entrada_count < MAX_DIGITOS) {
        entrada_atual[entrada_count] = digito;
        entrada_count++;
        entrada_atual[entrada_count] = '\0';
        printf("Digito adicionado: %c, Entrada atual: %s\n", digito, entrada_atual);
    }
}

// Função para gerar string de asteriscos para senha
void gerar_senha_asteriscos(char *buffer, size_t buffer_size, int tamanho) {
    int i;
    for (i = 0; i < tamanho && i < (int)buffer_size - 1; i++) {
        buffer[i] = '*';
    }
    buffer[i] = '\0';
}

// Função para verificar login
bool verificar_login(const char *login) {
    bool resultado = (strcmp(login, LOGIN_CORRETO) == 0);
    printf("Verificando login: %s == %s ? %s\n", login, LOGIN_CORRETO, resultado ? "SIM" : "NAO");
    return resultado;
}

// Função para verificar senha
bool verificar_senha(const char *senha) {
    bool resultado = (strcmp(senha, SENHA_CORRETA) == 0);
    printf("Verificando senha: %s == %s ? %s\n", senha, SENHA_CORRETA, resultado ? "SIM" : "NAO");
    return resultado;
}

// Função para atualizar o display OLED (100% original)
void atualizar_display(void) {
    char buffer[64];
    char senha_oculta[64];
    
    clear_display();
    
    // Título fixo
    display_text_no_clear("SISTEMA DE ACESSO", 5, 0, 1);
    ssd1306_draw_line(&display, 0, 12, 127, 12);
    
    switch (estado_atual) {
        case ESTADO_AGUARDANDO_LOGIN:
            display_text_no_clear("DIGITE SEU LOGIN:", 5, 20, 1);
            snprintf(buffer, sizeof(buffer), "> %s", entrada_atual);
            display_text_no_clear(buffer, 5, 35, 2);
            display_text_no_clear("[*] CONFIRMAR", 5, 55, 1);
            break;
            
        case ESTADO_VERIFICANDO_LOGIN:
            display_text_no_clear("VERIFICANDO LOGIN...", 10, 30, 1);
            break;
            
        case ESTADO_LOGIN_INVALIDO:
            display_text_no_clear("USUARIO NAO", 25, 20, 1);
            display_text_no_clear("CADASTRADO!", 25, 35, 1);
            display_text_no_clear("[#] TENTAR NOVAMENTE", 5, 55, 1);
            break;
            
        case ESTADO_AGUARDANDO_SENHA:
            snprintf(buffer, sizeof(buffer), "LOGIN: %s", login_usuario);
            display_text_no_clear(buffer, 5, 20, 1);
            display_text_no_clear("DIGITE SENHA:", 5, 35, 1);
            
            gerar_senha_asteriscos(senha_oculta, sizeof(senha_oculta), entrada_count);
            snprintf(buffer, sizeof(buffer), "> %s", senha_oculta);
            display_text_no_clear(buffer, 5, 45, 2);
            
            display_text_no_clear("[*] CONFIRMAR", 5, 60, 1);
            break;
            
        case ESTADO_VERIFICANDO_SENHA:
            display_text_no_clear("VERIFICANDO SENHA...", 10, 30, 1);
            break;
            
        case ESTADO_SENHA_INVALIDA:
            display_text_no_clear("SENHA INCORRETA!", 10, 20, 1);
            display_text_no_clear("[#] TENTAR NOVAMENTE", 5, 40, 1);
            break;
            
        case ESTADO_ACESSO_CONCEDIDO:
            snprintf(buffer, sizeof(buffer), "BEM-VINDO %s!", login_usuario);
            display_text_no_clear(buffer, 5, 20, 1);
            display_text_no_clear("ACESSO PERMITIDO", 10, 45, 1);
            break;
            
        case ESTADO_ERRO:
            display_text_no_clear("ERRO NO SISTEMA", 15, 25, 1);
            display_text_no_clear("REINICIANDO...", 20, 45, 1);
            break;
    }
    
    show_display();
}

// Função para agendar transição automática de estado
void agendar_transicao(EstadoSistema destino, uint32_t delay_ms) {
    aguardando_transicao = true;
    estado_destino = destino;
    tempo_transicao = make_timeout_time_ms(delay_ms);
}

// Função para processar confirmação (*)
void processar_confirmacao(PIO pio, uint sm) {
    switch (estado_atual) {
        case ESTADO_AGUARDANDO_LOGIN:
            if (entrada_count == MAX_DIGITOS) {
                printf("Confirmando login: %s\n", entrada_atual);
                estado_atual = ESTADO_VERIFICANDO_LOGIN;
                atualizar_display();
                
                if (verificar_login(entrada_atual)) {
                    strcpy(login_usuario, entrada_atual);
                    limpar_entrada();
                    
                    imprimir_desenho(simbolo_v, pio, sm);
                    beepCofre();
                    
                    agendar_transicao(ESTADO_AGUARDANDO_SENHA, 1000);
                } else {
                    limpar_entrada();
                    estado_atual = ESTADO_LOGIN_INVALIDO;
                    imprimir_desenho(simbolo_x, pio, sm);
                    beepCofreErro();
                }
                atualizar_display();
            } else {
                printf("Login incompleto! Digite %d digitos.\n", MAX_DIGITOS);
                beepCofreErro();
            }
            break;
            
        case ESTADO_AGUARDANDO_SENHA:
            if (entrada_count == MAX_DIGITOS) {
                printf("Confirmando senha: %s\n", entrada_atual);
                estado_atual = ESTADO_VERIFICANDO_SENHA;
                atualizar_display();
                
                if (verificar_senha(entrada_atual)) {
                    limpar_entrada();
                    
                    imprimir_desenho(simbolo_v, pio, sm);
                    
                    // Abre servo
                    servo_set_angle(SERVO_ABERTO);
                    servo_aberto = true;
                    
                    tocar_beep_vitoria = true;
                    agendar_transicao(ESTADO_ACESSO_CONCEDIDO, 200);
                } else {
                    limpar_entrada();
                    estado_atual = ESTADO_SENHA_INVALIDA;
                    imprimir_desenho(simbolo_x, pio, sm);
                    beepCofreErro();
                    atualizar_display();
                }
            } else {
                printf("Senha incompleta! Digite %d digitos.\n", MAX_DIGITOS);
                beepCofreErro();
            }
            break;
    }
}

// Função para processar cancelamento (#)
void processar_cancelamento(PIO pio, uint sm) {
    printf("Tecla # pressionada no estado: %d\n", estado_atual);
    
    switch (estado_atual) {
        case ESTADO_ACESSO_CONCEDIDO:
            // Fecha cofre imediatamente
            servo_set_angle(SERVO_FECHADO);
            servo_aberto = false;
            sleep_ms(100);
            
            estado_atual = ESTADO_AGUARDANDO_LOGIN;
            limpar_entrada();
            strcpy(login_usuario, "");
            imprimir_desenho(interrogacao, pio, sm);
            atualizar_display();
            printf("Cofre fechado e sistema reiniciado.\n");
            break;
            
        case ESTADO_LOGIN_INVALIDO:
        case ESTADO_SENHA_INVALIDA:
        case ESTADO_ERRO:
            estado_atual = ESTADO_AGUARDANDO_LOGIN;
            limpar_entrada();
            strcpy(login_usuario, "");
            imprimir_desenho(interrogacao, pio, sm);
            atualizar_display();
            printf("Sistema reiniciado.\n");
            break;
            
        case ESTADO_AGUARDANDO_LOGIN:
        case ESTADO_AGUARDANDO_SENHA:
            limpar_entrada();
            atualizar_display();
            printf("Entrada limpa.\n");
            break;
            
        default:
            estado_atual = ESTADO_AGUARDANDO_LOGIN;
            limpar_entrada();
            strcpy(login_usuario, "");
            imprimir_desenho(interrogacao, pio, sm);
            atualizar_display();
            break;
    }
}

// Função para ler sensor AHT10 e imprimir SOMENTE no terminal
void ler_sensor_terminal(void) {
    if (time_reached(ultimo_tempo_leitura)) {
        if (aht10_read(&temperatura, &umidade)) {
            printf("[AHT10] Temp: %.1f°C | Umid: %.1f%%\n", temperatura, umidade);
        } else {
            printf("[AHT10] ERRO: Falha na leitura do sensor\n");
        }
        ultimo_tempo_leitura = make_timeout_time_ms(INTERVALO_LEITURA_MS);
    }
}

// Função para enviar dados para dashboard via HTTP
void enviar_dados_para_dashboard(void) {
    static absolute_time_t ultimo_envio = {0};
    const uint32_t INTERVALO_ENVIO_MS = 5000; // Envia a cada 5 segundos
    
    if (time_reached(ultimo_envio)) {
        const char* status = servo_aberto ? "ABERTO" : "FECHADO";
        http_post_json(temperatura, umidade, status);
        ultimo_envio = make_timeout_time_ms(INTERVALO_ENVIO_MS);
    }
}

int main() {
    stdio_init_all();
    
    // === ETAPA 1: INICIALIZAÇÃO DO WiFi (DEVE SER A PRIMEIRA OPERAÇÃO!) ===
    printf("\n=== INICIALIZANDO WiFi (PRIMEIRA OPERACAO NO PICO W) ===\n");
    if (cyw43_arch_init()) {
        printf("FALHA: Não foi possível inicializar o WiFi\n");
        while (true) {
            sleep_ms(1000);
        }
    }
    
    cyw43_arch_enable_sta_mode();
    printf("Conectando à WiFi: %s\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 15000)) {
        printf("FALHA: Timeout na conexão WiFi (15s)\n");
        // Pisca LED vermelho em erro WiFi
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
        while (true) {
            gpio_put(LED_PIN, 1);
            sleep_ms(200);
            gpio_put(LED_PIN, 0);
            sleep_ms(200);
        }
    }
    
    const char* ip = ip4addr_ntoa(netif_ip4_addr(netif_default));
    printf("WiFi conectado! IP do Pico W: %s\n", ip);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); // LED azul ON
    
    // Inicializa cliente HTTP APÓS WiFi
    http_init();
    
    // === ETAPA 2: INICIALIZAÇÃO DOS PERIFÉRICOS (APÓS WiFi) ===
    printf("\n=== INICIALIZANDO PERIFERICOS ===\n");
    
    // GPIO para LED de status
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    // I2C0 EXPLÍCITO para Display OLED + Sensor AHT10 (GPIO 0=SDA, 1=SCL)
    printf("Configurando I2C0 (GPIO 0=SDA, 1=SCL)...\n");
    i2c_init(i2c0, 400000); // 400 kHz
    gpio_set_function(0, GPIO_FUNC_I2C);
    gpio_set_function(1, GPIO_FUNC_I2C);
    gpio_pull_up(0);
    gpio_pull_up(1);
    sleep_ms(100);
    
    // Inicializa módulos (ordem segura)
    buzzer_init();
    printf("Buzzer inicializado\n");
    
    teclado_init();
    printf("Teclado matricial inicializado\n");
    
    display_init();
    printf("Display OLED inicializado\n");
    
    servo_init();
    printf("Servo motor inicializado (GPIO 2)\n");
    
    aht10_i2c_init(); // Usa I2C0 já configurado
    aht10_init();
    printf("Sensor AHT10 inicializado\n");
    
    // Primeira leitura do sensor
    printf("\n=== CALIBRANDO SENSOR AHT10 ===\n");
    aht10_trigger_measurement();
    sleep_ms(100);
    if (aht10_read(&temperatura, &umidade)) {
        printf("[AHT10] OK - Temp: %.1f°C | Umid: %.1f%%\n", temperatura, umidade);
    } else {
        printf("[AHT10] ATENÇÃO: Falha na leitura inicial\n");
    }
    printf("================================\n\n");
    
    // Posiciona servo inicialmente FECHADO
    servo_set_angle(SERVO_FECHADO);
    servo_aberto = false;
    sleep_ms(500);
    
    // Mensagem inicial no display OLED
    clear_display();
    display_text("SISTEMA DE ACESSO", 5, 5, 1);
    display_text_no_clear("SEGURO", 35, 20, 1);
    display_text_no_clear("Embarcatech", 25, 40, 1);
    show_display();
    sleep_ms(2000);
    
    // Inicializa matriz de LEDs PIO
    PIO pio = pio0;
    uint sm = configurar_matriz(pio);
    printf("Matriz de LEDs 5x5 inicializada (PIO0)\n");
    
    // Estado inicial do sistema
    estado_atual = ESTADO_AGUARDANDO_LOGIN;
    limpar_entrada();
    strcpy(login_usuario, "");
    imprimir_desenho(interrogacao, pio, sm);
    atualizar_display();
    
    // Inicializa flags
    tocar_beep_vitoria = false;
    ultimo_tempo_leitura = make_timeout_time_ms(INTERVALO_LEITURA_MS);
    
    printf("=== SISTEMA PRONTO ===\n");
    printf("IP do Pico W: %s\n", ip);
    printf("Dashboard: http://%s:1880/ui\n", HTTP_SERVER_IP);
    printf("Login: %s | Senha: %s\n", LOGIN_CORRETO, SENHA_CORRETA);
    printf("=======================\n\n");
    
    // === LOOP PRINCIPAL ===
    while(true) {
        // ✅ POLLING DO WiFi DEVE SER A PRIMEIRA OPERAÇÃO NO LOOP!
        cyw43_arch_poll();  // Mantém stack de rede ativa - OBRIGATÓRIO!
        
        // Verifica transições automáticas agendadas
        if (aguardando_transicao && time_reached(tempo_transicao)) {
            estado_atual = estado_destino;
            aguardando_transicao = false;
            atualizar_display();
            
            printf("Transicao para estado: %d\n", estado_atual);
            
            // TOCA BEEP DE VITÓRIA
            if (estado_atual == ESTADO_ACESSO_CONCEDIDO && tocar_beep_vitoria) {
                tocar_beep_vitoria = false;
                printf(">>> VITORIA! Acesso autorizado!\n");
                beepVitoria();
            }
            
            // Após acesso concedido, mantém tela por 5s e reinicia
            if (estado_atual == ESTADO_ACESSO_CONCEDIDO) {
                agendar_transicao(ESTADO_AGUARDANDO_LOGIN, 5000);
            }
            
            // Ao reiniciar, FECHA O SERVO primeiro
            if (estado_atual == ESTADO_AGUARDANDO_LOGIN && estado_destino == ESTADO_AGUARDANDO_LOGIN) {
                if (servo_aberto) {
                    servo_set_angle(SERVO_FECHADO);
                    servo_aberto = false;
                    printf(">>> Cofre fechado automaticamente.\n");
                }
                limpar_entrada();
                strcpy(login_usuario, "");
                imprimir_desenho(interrogacao, pio, sm);
                atualizar_display();
            }
        }
        
        // Lê sensor e envia para dashboard
        ler_sensor_terminal();
        enviar_dados_para_dashboard();
        
        // Pisca LED indicando sistema ativo
        blink_led();
        
        // Lê teclado (agora deve funcionar corretamente)
        char pressed_key = teclado_read();
        if(pressed_key != 0) {
            aguardando_transicao = false;
            tocar_beep_vitoria = false;
            
            printf("Tecla: '%c' (Estado: %d)\n", pressed_key, estado_atual);
            beepCofre();
            
            if (pressed_key >= '0' && pressed_key <= '9') {
                if (estado_atual == ESTADO_AGUARDANDO_LOGIN || 
                    estado_atual == ESTADO_AGUARDANDO_SENHA) {
                    adicionar_digito(pressed_key);
                    atualizar_display();
                }
            }
            else if (pressed_key == '*') {
                processar_confirmacao(pio, sm);
            }
            else if (pressed_key == '#') {
                processar_cancelamento(pio, sm);
            }
            else {
                printf("Tecla desconhecida: '%c'\n", pressed_key);
            }
        }
        
        // Polling rápido para resposta do teclado + manutenção WiFi
        sleep_ms(20);
    }
    
    return 0;
}
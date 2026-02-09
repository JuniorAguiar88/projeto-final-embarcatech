# Generated Cmake Pico project file
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

# Configuração obrigatória para Pico W (WiFi)
set(PICO_BOARD pico_w CACHE STRING "Board type")
set(PICO_STDIO_USB 1)  # Saída serial via USB para monitoramento

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(projeto_final C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Adiciona executável com TODOS os arquivos fonte necessários
add_executable(projeto_final
    projeto_final.c
    matriz_leds.c
    teclado.c
    buzzer.c
    ssd1306.c
    display.c
    servo_motor.c
    aht10.c
    http_post.c
)

# Define nome e versão do programa
pico_set_program_name(projeto_final "projeto_final")
pico_set_program_version(projeto_final "0.1")

# Gera cabeçalho PIO para matriz de LEDs
pico_generate_pio_header(projeto_final ${CMAKE_CURRENT_LIST_DIR}/pio_matrix.pio)

# Configura saída serial (USB ativado, UART desativado)
pico_enable_stdio_uart(projeto_final 0)
pico_enable_stdio_usb(projeto_final 1)

# Definições de compilação específicas para Pico W + MQTT
target_compile_definitions(projeto_final PRIVATE
    PICO_BOARD=\"pico_w\"
    PICO_STDIO_USB=1
    LWIP_MQTT_SUPPORT=1
)

# Diretórios de inclusão
target_include_directories(projeto_final PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Bibliotecas necessárias (incluindo suporte WiFi/MQTT)
target_link_libraries(projeto_final
    pico_stdlib
    pico_multicore
    pico_cyw43_arch_lwip_threadsafe_background  # <<<<< ESSENCIAL PARA WiFi + MQTT <<<<<
    hardware_pio
    hardware_clocks
    hardware_i2c
    hardware_pwm
    hardware_gpio
)

# Gera arquivos binários adicionais (.uf2, .hex, .bin)
pico_add_extra_outputs(projeto_final)
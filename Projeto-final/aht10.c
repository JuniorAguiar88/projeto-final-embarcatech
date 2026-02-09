#include "aht10.h"

void aht10_i2c_init(){
    i2c_init(AHT10_I2C_PORT, 400 * 1000); // 400kHz
    gpio_set_function(AHT10_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(AHT10_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(AHT10_I2C_SDA_PIN);
    gpio_pull_up(AHT10_I2C_SCL_PIN);
}

void aht10_init() {
    // Comando de inicialização/calibração
    uint8_t init_cmd[] = {0xBE, 0x08, 0x00};
    i2c_write_blocking(AHT10_I2C_PORT, AHT10_ADDR, init_cmd, 3, false);
    sleep_ms(20);
}

void aht10_trigger_measurement() {
    uint8_t cmd[] = {0xAC, 0x33, 0x00};
    i2c_write_blocking(AHT10_I2C_PORT, AHT10_ADDR, cmd, 3, false);
}

bool aht10_read(float *temperature, float *humidity) {
    // Dispara medição
    aht10_trigger_measurement();
    sleep_ms(80); // Tempo mínimo de conversão
    
    // Lê 6 bytes de dados
    uint8_t data[6];
    int res = i2c_read_blocking(AHT10_I2C_PORT, AHT10_ADDR, data, 6, false);
    if (res != 6) {
        return false;
    }

    // Verifica se medição está completa (bit 7 do byte 0 deve ser 0)
    if (data[0] & 0x80) {
        return false;
    }

    // Extrai valores brutos de acordo com datasheet
    uint32_t hum_raw = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | ((data[3] >> 4) & 0x0F);
    uint32_t temp_raw = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];

    // Converte para valores físicos
    *humidity = (hum_raw * 100.0f) / 1048576.0f;
    *temperature = (temp_raw * 200.0f) / 1048576.0f - 50.0f;
    
    // Validação de faixa razoável
    if (*humidity < 0.0f || *humidity > 100.0f || *temperature < -40.0f || *temperature > 85.0f) {
        return false;
    }
    
    return true;
}
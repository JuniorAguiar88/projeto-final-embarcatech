// mqtt_config.h
#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

// Configurações de WiFi
#define WIFI_SSID       "VIVOFIBRA-4E51"      // <<<<< ALTERE AQUI <<<<<
#define WIFI_PASSWORD   "gF7v6oT74b"     // <<<<< ALTERE AQUI <<<<<

// Configurações MQTT
#define MQTT_BROKER     "broker.hivemq.com"  // Broker público gratuito (sem autenticação)
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "cofre_pico_w_01"
#define MQTT_TOPIC_TEMP "cofre/temperatura"
#define MQTT_TOPIC_UMID "cofre/umidade"
#define MQTT_TOPIC_STATUS "cofre/status"

// Intervalo de envio (ms)
#define MQTT_INTERVAL_MS 5000  // Envia a cada 5 segundos

#endif // MQTT_CONFIG_H
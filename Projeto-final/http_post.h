#ifndef HTTP_POST_H
#define HTTP_POST_H

// ⚠️ ALTERE ESTES VALORES PARA SEU AMBIENTE ⚠️
#define WIFI_SSID       "VIVOFIBRA-4E51"      // <<<<< SUA REDE WiFi <<<<<
#define WIFI_PASSWORD   "gF7v6oT74b"     // <<<<< SUA SENHA WiFi <<<<<
#define HTTP_SERVER_IP  "192.168.15.3"      // <<<<< IP DO SEU COMPUTADOR <<<<<


#include <stdbool.h>

void http_init(void);

bool http_post_json(float temperatura, float umidade, const char* status);

#endif // HTTP_POST_H
// http_client.h
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <stdbool.h>

bool http_post_data(float temperatura, float umidade, const char* status);
void http_init(void);

#endif // HTTP_CLIENT_H
#ifndef _DHT11_H_
#define _DHT11_H_

#include <stdint.h>

void delay_us(uint32_t us);
void dht11_read(uint8_t *data);

#endif /* _DHT11_H_ */

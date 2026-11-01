#ifndef __CONSOLE_IO_H
#define __CONSOLE_IO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void console_init(void);
void forth_tx_char(uint8_t c);
uint8_t forth_rx_char(void);
int32_t forth_rx_ready(void);

#ifdef __cplusplus
}
#endif

#endif /* __CONSOLE_IO_H */

#ifndef VL53L0X_C_H
#define VL53L0X_C_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void vl53l0x_init(void);
void vl53l0x_start(uint32_t period_ms);
uint16_t vl53l0x_read_mm(void);
bool vl53l0x_timeout(void);

#ifdef __cplusplus
}
#endif

#endif

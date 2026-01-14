#include "vl53l0x_c.h"
#include "VL53L0X.h"

static VL53L0X sensor;

extern "C" {

void vl53l0x_init(void) {
    sensor.init();
    sensor.setTimeout(500);
}

void vl53l0x_start(uint32_t period_ms) {
    sensor.startContinuous(period_ms);
}

uint16_t vl53l0x_read_mm(void) {
    return sensor.readRangeContinuousMillimeters();
}

bool vl53l0x_timeout(void) {
    return sensor.timeoutOccurred();
}

}

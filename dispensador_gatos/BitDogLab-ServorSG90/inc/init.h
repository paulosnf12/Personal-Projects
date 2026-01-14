// init.h
#ifndef INIT_H
#define INIT_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "servo.h"

// inicializa todo o sistema: stdio, I2C, display e sensor
void initializeSystem(void);

#endif // INIT_H

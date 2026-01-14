// init.c
#include "init.h"

void initializeSystem(void) {
    stdio_init_all();
    sleep_ms(1000);

    // inicializa o servo
    servo_init();
}



#include <Arduino.h>
#include "buffer.h"
#include "serial_cmd.h"
#include "timer.h"

void setup()
{
    buffer_init();
    serial_cmd_init();
    initTimer();
}

void loop()
{
    serial_cmd_poll();
}

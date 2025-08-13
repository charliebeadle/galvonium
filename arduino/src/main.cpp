#include <Arduino.h>
#include "buffer.h"
#include "serial_cmd.h"

void setup()
{
    buffer_init();
    serial_cmd_init();
}

void loop()
{
    serial_cmd_poll();
}

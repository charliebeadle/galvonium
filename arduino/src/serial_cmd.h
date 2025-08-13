#pragma once

void serial_cmd_init();
void serial_cmd_poll();
void process_serial_command(const char *cmd);

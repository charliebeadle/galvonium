#ifndef SERIAL_CMD_H
#define SERIAL_CMD_H

#include <Arduino.h>

// ========== CONSTANTS ==========
#define SERIAL_BUFFER_SIZE 24
#define PARSE_BUFFER_SIZE 12

// ========== ENUMS ==========
enum CommandType {
  CMD_WRITE,
  CMD_CLEAR,
  CMD_SWAP,
  CMD_DUMP,
  CMD_SIZE,
  CMD_HELP,
  CMD_CONFIG,
  CMD_EEPROM,
  CMD_UNKNOWN
};

// ========== FUNCTION DECLARATIONS ==========

// Initialize serial communication
void serial_cmd_init();

// Poll for new serial commands
void serial_cmd_poll();

// Parse a command string and return its type
CommandType parse_command(const char *cmd, const char **arg_start);

// Process a complete serial command
void process_serial_command(const char *cmd);

#endif // SERIAL_CMD_H

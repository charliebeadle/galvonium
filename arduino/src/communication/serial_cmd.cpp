#include "serial_cmd.h"
#include "../config/config.h"
#include "../core/timer.h"
#include "../modes/buffer.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <string.h>

#define SERIAL_BUFFER_SIZE 32 // Reduced from 64, saves 32 bytes

static char serial_buf[SERIAL_BUFFER_SIZE];
static int serial_buf_pos = 0;

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

// Command names and help - only command names need to be in PROGMEM for parsing
const char cmd_write[] PROGMEM = "WRITE";
const char cmd_clear[] PROGMEM = "CLEAR";
const char cmd_swap[] PROGMEM = "SWAP";
const char cmd_dump[] PROGMEM = "DUMP";
const char cmd_size[] PROGMEM = "SIZE";
const char cmd_help[] PROGMEM = "HELP";
const char cmd_config[] PROGMEM = "CONFIG";
const char cmd_eeprom[] PROGMEM = "EEPROM";

// --- Command table stored in PROGMEM ---
struct CommandEntry {
  const char *name;
  CommandType type;
};

static const CommandEntry command_table[] PROGMEM = {
    {cmd_write, CMD_WRITE},   {cmd_clear, CMD_CLEAR},  {cmd_swap, CMD_SWAP},
    {cmd_dump, CMD_DUMP},     {cmd_size, CMD_SIZE},    {cmd_help, CMD_HELP},
    {cmd_config, CMD_CONFIG}, {cmd_eeprom, CMD_EEPROM}};
static const int NUM_COMMANDS =
    sizeof(command_table) / sizeof(command_table[0]);

// --- Forward declarations of handler functions ---
static void handle_write(const char *args);
static void handle_clear(const char *args);
static void handle_swap(const char *args);
static void handle_dump(const char *args);
static void handle_size(const char *args);
static void handle_help(const char *args);
static void handle_config(const char *args);
static void handle_eeprom(const char *args);

// --- Helper function to find parameter by name ---
static ConfigParam find_param_by_name(const char *name) {
  for (int i = 0; i < PARAM_COUNT; i++) {
    const char *param_name = config_get_param_name((ConfigParam)i);
    if (strcmp(name, param_name) == 0) {
      return (ConfigParam)i;
    }
  }
  return PARAM_COUNT; // Invalid parameter
}

// --- Serial interface setup ---
void serial_cmd_init() {
  Serial.begin(9600);
  Serial.println(F("Galvonium buffer test ready."));
}

// --- Poll for new serial commands ---
void serial_cmd_poll() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serial_buf_pos > 0) {
        serial_buf[serial_buf_pos] = 0; // Null-terminate
        process_serial_command(serial_buf);
        serial_buf_pos = 0;
      }
    } else if (serial_buf_pos < SERIAL_BUFFER_SIZE - 1) {
      serial_buf[serial_buf_pos++] = c;
    }
  }
}

// --- Command parsing ---
CommandType parse_command(const char *cmd, const char **arg_start) {
  while (*cmd == ' ')
    cmd++; // Skip leading spaces

  for (int i = 0; i < NUM_COMMANDS; ++i) {
    // Read command name from PROGMEM
    const char *prog_name = (const char *)pgm_read_word(&command_table[i].name);
    size_t n = strlen_P(prog_name);

    if (strncmp_P(cmd, prog_name, n) == 0 &&
        (cmd[n] == '\0' || cmd[n] == ' ')) {
      if (arg_start)
        *arg_start = cmd + n;
      return (CommandType)pgm_read_byte(&command_table[i].type);
    }
  }
  if (arg_start)
    *arg_start = cmd;
  return CMD_UNKNOWN;
}

// --- Command dispatch using switch instead of function pointers ---
void process_serial_command(const char *cmd) {
  const char *args = nullptr;
  CommandType type = parse_command(cmd, &args);

  switch (type) {
  case CMD_WRITE:
    handle_write(args);
    break;
  case CMD_CLEAR:
    handle_clear(args);
    break;
  case CMD_SWAP:
    handle_swap(args);
    break;
  case CMD_DUMP:
    handle_dump(args);
    break;
  case CMD_SIZE:
    handle_size(args);
    break;
  case CMD_HELP:
    handle_help(args);
    break;
  case CMD_CONFIG:
    handle_config(args);
    break;
  case CMD_EEPROM:
    handle_eeprom(args);
    break;
  default:
    Serial.println(F("ERR: Unknown command"));
    break;
  }
}

// --- Command Handlers ---
static void handle_write(const char *args) {
  int idx, x, y, flags;
  char modifier[16] = "";

  // Parse required parameters first, then optional modifier
  int parsed = sscanf(args, "%d %d %d %d %s", &idx, &x, &y, &flags, modifier);

  if (parsed < 4) {
    Serial.println(F("ERR: Usage WRITE idx x y flags [ACTIVE|INACTIVE]"));
    return;
  }

  bool use_active = false;
  if (parsed == 5) {
    if (strcmp(modifier, "ACTIVE") == 0) {
      use_active = true;
    } else if (strcmp(modifier, "INACTIVE") == 0) {
      use_active = false;
    } else {
      Serial.println(F("ERR: Buffer modifier must be ACTIVE or INACTIVE"));
      return;
    }
  }

  // Select target buffer
  volatile Step *target_buffer = use_active ? buffer_active : buffer_inactive;

  if (buffer_write(target_buffer, idx, x, y, flags) == 0) {
    Serial.println(use_active ? F("OK (active buffer modified!)") : F("OK"));
  } else {
    Serial.println(F("ERR: Index out of range"));
  }
}

static void handle_clear(const char *args) {
  char modifier[16] = "";
  int parsed = sscanf(args, "%s", modifier);

  bool use_active = false;
  if (parsed == 1) {
    if (strcmp(modifier, "ACTIVE") == 0) {
      use_active = true;
    } else if (strcmp(modifier, "INACTIVE") == 0) {
      use_active = false;
    } else {
      Serial.println(F("ERR: Buffer modifier must be ACTIVE or INACTIVE"));
      return;
    }
  }

  if (use_active) {
    buffer_clear(buffer_active);
    buffer_active_steps = 0;
    Serial.println(F("OK (active buffer cleared!)"));
  } else {
    buffer_clear(buffer_inactive);
    buffer_inactive_steps = 0;
    Serial.println(F("OK"));
  }
}

static void handle_swap(const char *) {
  requestBufferSwap();
  Serial.println(F("OK"));
}

static void handle_dump(const char *args) {
  char modifier[16] = "";
  int parsed = sscanf(args, "%s", modifier);

  bool use_active = false;
  if (parsed == 1) {
    if (strcmp(modifier, "ACTIVE") == 0) {
      use_active = true;
    } else if (strcmp(modifier, "INACTIVE") == 0) {
      use_active = false;
    } else {
      Serial.println(F("ERR: Buffer modifier must be ACTIVE or INACTIVE"));
      Serial.println(F("EOC"));
      return;
    }
  }

  // Select source buffer and step count
  volatile Step *source_buffer = use_active ? buffer_active : buffer_inactive;
  int steps = use_active ? buffer_active_steps : buffer_inactive_steps;
  const char *buffer_name = use_active ? "ACTIVE" : "INACTIVE";

  Serial.print(F("DUMP START ("));
  Serial.print(buffer_name);
  Serial.println(F(")"));
  Serial.print(F("Buffer Steps: "));
  Serial.println(steps);

  for (int i = 0; i < steps; ++i) {

    Serial.print(i);
    Serial.print(F(": "));
    Serial.print(source_buffer[i].x);
    Serial.print(F(","));
    Serial.print(source_buffer[i].y);
    Serial.print(F(" "));
    Serial.println(source_buffer[i].flags);
  }

  Serial.println(F("DUMP END"));
  Serial.println(F("EOC"));
}

static void handle_size(const char *args) {
  int n;
  char modifier[16] = "";

  int parsed = sscanf(args, "%d %s", &n, modifier);

  if (parsed < 1 || n < 0 || n > MAX_STEPS) {
    Serial.println(F("ERR: Usage SIZE n [ACTIVE|INACTIVE]"));
    return;
  }

  bool use_active = false;
  if (parsed == 2) {
    if (strcmp(modifier, "ACTIVE") == 0) {
      use_active = true;
    } else if (strcmp(modifier, "INACTIVE") == 0) {
      use_active = false;
    } else {
      Serial.println(F("ERR: Buffer modifier must be ACTIVE or INACTIVE"));
      return;
    }
  }

  if (use_active) {
    buffer_active_steps = n;
    Serial.println(F("OK (active buffer size changed!)"));
  } else {
    buffer_inactive_steps = n;
    Serial.println(F("OK"));
  }
}

static void handle_help(const char *) {
  // Print help text directly from PROGMEM using Serial.println(F())
  Serial.println(F("Galvonium Serial Commands:"));
  Serial.println(F("  WRITE idx x y flags [ACTIVE|INACTIVE] - Write step "
                   "(default: inactive)"));
  Serial.println(F("  CLEAR [ACTIVE|INACTIVE]               - Clear buffer "
                   "(default: inactive)"));
  Serial.println(F("  SWAP                                  - Atomically swap "
                   "active/inactive buffers"));
  Serial.println(F("  DUMP [ACTIVE|INACTIVE]                - Dump buffer "
                   "(default: inactive)"));
  Serial.println(F("  SIZE n [ACTIVE|INACTIVE]              - Set buffer size "
                   "(default: inactive)"));
  Serial.println(F("  CONFIG [GET|SET|RESET] [PARAM] [VALUE] - Get/set "
                   "configuration parameters (MODE, PPS, LASER_DELAY, "
                   "DWELL_US, DEBUG_FLAGS)"));
  Serial.println(F("  EEPROM [READ|WRITE|DUMP] - EEPROM operations"));
  Serial.println(
      F("  HELP                                  - Show this help message"));
  Serial.println(F("EOC"));
}

static void handle_config(const char *args) {
  char subcmd[16] = "";
  char param[16] = "";
  int value = 0;

  // Parse command: CONFIG [GET|SET] [PARAM] [VALUE]
  int parsed = sscanf(args, "%s %s %d", subcmd, param, &value);

  if (parsed < 1) {
    // Just "CONFIG" - show all parameters
    Serial.println(F("Current Configuration (RAM):"));

    // Automatically iterate through all config parameters
    for (uint8_t i = 0; i < PARAM_COUNT; i++) {
      ConfigParam current_param = (ConfigParam)i;
      Serial.print(F("  "));
      Serial.print(config_get_param_name(current_param));
      Serial.print(F(": "));
      Serial.println(config_get(current_param));
    }

    Serial.print(F("  EEPROM Size: "));
    Serial.println(EEPROM.length());
    Serial.print(F("  Config Size: "));
    Serial.println(sizeof(GalvoConfig));
    Serial.println(F("EOC"));
    return;
  }

  if (strcmp(subcmd, "GET") == 0) {
    if (parsed < 2) {
      Serial.println(F("ERR: Usage CONFIG GET <PARAM>"));
      return;
    }

    // Find parameter by name
    ConfigParam target_param = find_param_by_name(param);
    if (target_param == PARAM_COUNT) {
      Serial.println(F("ERR: Unknown parameter"));
      return;
    }

    Serial.print(param);
    Serial.print(F(": "));
    Serial.println(config_get(target_param));

  } else if (strcmp(subcmd, "SET") == 0) {
    if (parsed < 3) {
      Serial.println(F("ERR: Usage CONFIG SET <PARAM> <VALUE>"));
      return;
    }

    // Find parameter by name
    ConfigParam target_param = find_param_by_name(param);
    if (target_param == PARAM_COUNT) {
      Serial.println(F("ERR: Unknown parameter"));
      return;
    }

    // Set the parameter
    bool success = config_set(target_param, value);
    if (success) {
      Serial.println(
          F("OK - Parameter updated in RAM (use EEPROM WRITE to save)"));
    } else {
      Serial.println(F("ERR: Invalid value"));
    }

  } else if (strcmp(subcmd, "RESET") == 0) {
    // Reset configuration to defaults (RAM only)
    Serial.println(F("Resetting configuration to defaults..."));
    config_load_defaults();
    Serial.println(
        F("OK - Configuration reset to defaults (use EEPROM WRITE to save)"));

  } else {
    Serial.println(F("ERR: Usage CONFIG [GET|SET|RESET] [PARAM] [VALUE]"));
  }
}

static void handle_eeprom(const char *args) {
  char subcmd[16] = "";

  // Parse command: EEPROM [READ|WRITE|DUMP]
  int parsed = sscanf(args, "%s", subcmd);

  if (parsed < 1) {
    Serial.println(F("ERR: Usage EEPROM [READ|WRITE|DUMP]"));
    return;
  }

  if (strcmp(subcmd, "READ") == 0) {
    // Read configuration from EEPROM into RAM
    Serial.println(F("Reading configuration from EEPROM..."));
    if (config_load_from_eeprom()) {
      Serial.println(F("OK - Configuration loaded from EEPROM"));
    } else {
      Serial.println(F("ERR: Failed to read from EEPROM"));
    }

  } else if (strcmp(subcmd, "WRITE") == 0) {
    // Write current RAM configuration to EEPROM
    Serial.println(F("Writing configuration to EEPROM..."));
    if (config_save_to_eeprom()) {
      Serial.println(F("OK - Configuration saved to EEPROM"));
    } else {
      Serial.println(F("ERR: Failed to write to EEPROM"));
    }

  } else if (strcmp(subcmd, "DUMP") == 0) {
    // Show raw EEPROM contents for debugging
    Serial.println(F("EEPROM Debug Info:"));
    Serial.print(F("  EEPROM Size: "));
    Serial.println(EEPROM.length());
    Serial.print(F("  Config Size: "));
    Serial.println(sizeof(GalvoConfig));
    Serial.print(F("  Start Address: "));
    Serial.println(CONFIG_EEPROM_START);

    Serial.println(F("Raw EEPROM Contents:"));
    for (size_t i = 0; i < sizeof(GalvoConfig); i++) {
      uint8_t byte_val = EEPROM.read(CONFIG_EEPROM_START + i);
      Serial.print(F("  ["));
      Serial.print(i);
      Serial.print(F("]: 0x"));
      if (byte_val < 0x10)
        Serial.print(F("0"));
      Serial.println(byte_val, HEX);
    }
    Serial.println(F("EOC"));

  } else {
    Serial.println(F("ERR: Usage EEPROM [READ|WRITE|DUMP]"));
  }
}
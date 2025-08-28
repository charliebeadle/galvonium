#include "serial_cmd.h"
#include "../config/config.h"
#include "../config/debug.h"
#include "../config/eeprom.h"
#include "../core/timer.h"
#include "../modes/buffer.h"
#include <avr/pgmspace.h>
#include <string.h>

// ========== STATIC VARIABLES ==========
// These are now defined in globals.cpp

// ========== KEY OPTIMIZATION: SHARED PARSE BUFFER ==========
// Instead of each function having its own char arrays, share one
// g_parse_buf is now defined in globals.cpp

// ========== KEY OPTIMIZATION: SIMPLE INTEGER PARSER ==========
// Replaces sscanf for integer parsing - saves ~100 bytes of stack
static int parse_next_int(const char *&ptr) {
  // Skip spaces
  while (*ptr == ' ')
    ptr++;

  // Handle sign
  bool negative = false;
  if (*ptr == '-') {
    negative = true;
    ptr++;
  }

  // Parse digits
  int value = 0;
  while (*ptr >= '0' && *ptr <= '9') {
    value = value * 10 + (*ptr - '0');
    ptr++;
  }

  return negative ? -value : value;
}

// Extract next word into buffer, return true if found
static bool extract_word(const char *&ptr, char *dest, size_t max_len) {
  // Skip leading spaces
  while (*ptr == ' ')
    ptr++;

  if (*ptr == '\0')
    return false;

  size_t i = 0;
  while (*ptr && *ptr != ' ' && i < max_len - 1) {
    dest[i++] = *ptr++;
  }
  dest[i] = '\0';

  return i > 0;
}

// Command names and help - only command names need to be in PROGMEM for parsing
const char cmd_write[] PROGMEM = "WRITE";
const char cmd_clear[] PROGMEM = "CLEAR";
const char cmd_swap[] PROGMEM = "SWAP";
const char cmd_dump[] PROGMEM = "DUMP";
const char cmd_size[] PROGMEM = "SIZE";
const char cmd_help[] PROGMEM = "HELP";
const char cmd_config[] PROGMEM = "CONFIG";
const char cmd_eeprom[] PROGMEM = "EEPROM";
const char cmd_flags[] PROGMEM = "FLAGS";
const char cmd_debug[] PROGMEM = "DEBUG";

// --- Command table stored in PROGMEM ---

static const CommandEntry command_table[] PROGMEM = {
    {cmd_write, CMD_WRITE},   {cmd_clear, CMD_CLEAR},   {cmd_swap, CMD_SWAP},
    {cmd_dump, CMD_DUMP},     {cmd_size, CMD_SIZE},     {cmd_help, CMD_HELP},
    {cmd_config, CMD_CONFIG}, {cmd_eeprom, CMD_EEPROM}, {cmd_flags, CMD_FLAGS},
    {cmd_debug, CMD_DEBUG}};
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
static void handle_flags(const char *args);
static void handle_debug(const char *args);

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
  Serial.println(F("Galvonium ready."));
}

// --- Poll for new serial commands ---
void serial_cmd_poll() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      if (g_serial_buf_pos > 0) {
        g_serial_buf[g_serial_buf_pos] = 0; // Null-terminate
        process_serial_command(g_serial_buf);
        g_serial_buf_pos = 0;
      }
    } else if (g_serial_buf_pos < SERIAL_BUFFER_SIZE - 1) {
      g_serial_buf[g_serial_buf_pos++] = c;
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
  case CMD_FLAGS:
    handle_flags(args);
    break;
  case CMD_DEBUG:
    handle_debug(args);
    break;
  default:
    Serial.println(F("ERR: Unknown command"));
    break;
  }
}

// --- Command Handlers ---
static void handle_write(const char *args) {
  // ========== OPTIMIZED: No sscanf, no local arrays ==========
  const char *ptr = args;
  int idx = parse_next_int(ptr);
  int x = parse_next_int(ptr);
  int y = parse_next_int(ptr);
  int flags = parse_next_int(ptr);

  // Extract modifier using shared buffer
  bool has_modifier = extract_word(ptr, g_parse_buf, sizeof(g_parse_buf));

  // Validate inputs
  if (idx < 0 || x < 0 || y < 0) {
    Serial.println(F("ERR: Usage WRITE idx x y flags [ACTIVE|INACTIVE]"));
    return;
  }

  bool use_active = false;
  if (has_modifier) {
    char modifier_first = g_parse_buf[0];
    switch (modifier_first) {
    case 'A': // ACTIVE
      use_active = true;
      break;
    case 'I': // INACTIVE
      use_active = false;
      break;
    default:
      Serial.println(F("ERR: Buffer modifier must be ACTIVE or INACTIVE"));
      return;
    }
  }

  // Select target buffer
  volatile Step *target_buffer =
      use_active ? g_buffer_active : g_buffer_inactive;

  if (buffer_write(target_buffer, idx, x, y, flags) == 0) {
    Serial.print(idx);
    Serial.print(F(": "));
    Serial.print(x);
    Serial.print(F(", "));
    Serial.print(y);
    Serial.print(F(","));
    Serial.print(flags);
    Serial.println(use_active ? F(" OK (active buffer modified!)") : F(" OK"));
  } else {
    Serial.println(F("ERR: Index out of range"));
  }
}

static void handle_clear(const char *args) {
  // ========== OPTIMIZED: Reuse shared buffer ==========
  const char *ptr = args;
  bool has_modifier = extract_word(ptr, g_parse_buf, sizeof(g_parse_buf));

  bool use_active = false;
  if (has_modifier) {
    char modifier_first = g_parse_buf[0];
    switch (modifier_first) {
    case 'A': // ACTIVE
      use_active = true;
      break;
    case 'I': // INACTIVE
      use_active = false;
      break;
    default:
      Serial.println(F("ERR: Buffer modifier must be ACTIVE or INACTIVE"));
      return;
    }
  }

  if (use_active) {
    buffer_clear(g_buffer_active);
    g_buffer_active_steps = 0;
    Serial.println(F("OK (active buffer cleared!)"));
  } else {
    buffer_clear(g_buffer_inactive);
    g_buffer_inactive_steps = 0;
    Serial.println(F("OK"));
  }
}

static void handle_swap(const char *) {
  requestBufferSwap();
  Serial.println(F("OK"));
}

static void handle_dump(const char *args) {
  // ========== OPTIMIZED: Reuse shared buffer ==========
  const char *ptr = args;
  bool has_modifier = extract_word(ptr, g_parse_buf, sizeof(g_parse_buf));

  bool use_active = false;
  if (has_modifier) {
    char modifier_first = g_parse_buf[0];
    switch (modifier_first) {
    case 'A': // ACTIVE
      use_active = true;
      break;
    case 'I': // INACTIVE
      use_active = false;
      break;
    default:
      Serial.println(F("ERR: Buffer modifier must be ACTIVE or INACTIVE"));
      Serial.println(F("EOC"));
      return;
    }
  }

  // Select source buffer and step count
  volatile Step *source_buffer =
      use_active ? g_buffer_active : g_buffer_inactive;
  int steps = use_active ? g_buffer_active_steps : g_buffer_inactive_steps;
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
  // ========== OPTIMIZED: No sscanf ==========
  const char *ptr = args;
  int n = parse_next_int(ptr);

  // Extract modifier using shared buffer
  bool has_modifier = extract_word(ptr, g_parse_buf, sizeof(g_parse_buf));

  // Use config value instead of macro for validation
  uint8_t max_index = g_config.max_buffer_index;
  if (n < 0 || n > (max_index + 1)) {
    Serial.println(F("ERR: Usage SIZE n [ACTIVE|INACTIVE]"));
    return;
  }

  bool use_active = false;
  if (has_modifier) {
    char modifier_first = g_parse_buf[0];
    switch (modifier_first) {
    case 'A': // ACTIVE
      use_active = true;
      break;
    case 'I': // INACTIVE
      use_active = false;
      break;
    default:
      Serial.println(F("ERR: Buffer modifier must be ACTIVE or INACTIVE"));
      return;
    }
  }

  if (use_active) {
    g_buffer_active_steps = n;
    Serial.println(F("OK (active buffer size changed!)"));
  } else {
    g_buffer_inactive_steps = n;
    Serial.println(F("OK"));
  }
}

static void handle_help(const char *) {
  // Keep original help text - F() macro is fine for PROGMEM storage
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
                   "configuration parameters (MODE, PPS, MAX_BUFFER_INDEX, "
                   "MAX_STEP_LENGTH, DEBUG_FLAGS)"));
  Serial.println(F("  EEPROM [READ|WRITE|DUMP] - EEPROM operations"));
  Serial.println(
      F("  HELP                                  - Show this help message"));
  Serial.println(F("EOC"));
}

static void handle_config(const char *args) {
  // ========== OPTIMIZED: Use shared buffer, no sscanf ==========
  const char *ptr = args;

  // Get subcommand
  bool has_subcmd = extract_word(ptr, g_parse_buf, sizeof(g_parse_buf));

  if (!has_subcmd) {
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
    Serial.println(EEPROM_CONFIG_SIZE);
    Serial.print(F("  Config Size: "));
    Serial.println(sizeof(GalvoConfig));
    Serial.println(F("EOC"));
    return;
  }

  // Use first character for fast comparison
  char subcmd_first = g_parse_buf[0];

  // Declare variables outside switch to avoid redeclaration errors
  ConfigParam target_param;
  int value;
  bool success;

  switch (subcmd_first) {
  case 'G': // GET
    // Get parameter name
    if (!extract_word(ptr, g_parse_buf, sizeof(g_parse_buf))) {
      Serial.println(F("ERR: Usage CONFIG GET <PARAM>"));
      return;
    }

    // Find parameter by name
    target_param = find_param_by_name(g_parse_buf);
    if (target_param == PARAM_COUNT) {
      Serial.println(F("ERR: Unknown parameter"));
      return;
    }

    Serial.print(g_parse_buf);
    Serial.print(F(": "));
    Serial.println(config_get(target_param));
    break;

  case 'S': // SET
    // Get parameter name
    if (!extract_word(ptr, g_parse_buf, sizeof(g_parse_buf))) {
      Serial.println(F("ERR: Usage CONFIG SET <PARAM> <VALUE>"));
      return;
    }

    // Find parameter by name
    target_param = find_param_by_name(g_parse_buf);
    if (target_param == PARAM_COUNT) {
      Serial.println(F("ERR: Unknown parameter"));
      return;
    }

    // Get value
    value = parse_next_int(ptr);

    // Set the parameter
    success = config_set(target_param, value);
    if (success) {
      Serial.println(
          F("OK - Parameter updated in RAM (use EEPROM WRITE to save)"));
    } else {
      Serial.println(F("ERR: Invalid value"));
    }
    break;

  case 'R': // RESET
    // Reset configuration to defaults (RAM only)
    Serial.println(F("Resetting configuration to defaults..."));
    config_load_defaults();
    Serial.println(
        F("OK - Configuration reset to defaults (use EEPROM WRITE to save)"));
    break;

  default:
    Serial.println(F("ERR: Usage CONFIG [GET|SET|RESET] [PARAM] [VALUE]"));
    break;
  }
}

static void handle_flags(const char *args) {
  // ========== OPTIMIZED: Use shared buffer ==========
  const char *ptr = args;

  int flag;
  int value;

  // Get subcommand
  bool has_subcmd = extract_word(ptr, g_parse_buf, sizeof(g_parse_buf));

  if (!has_subcmd) {
    Serial.println(F("ERR: Usage FLAGS [GET|SET] [FLAG] [VALUE]"));
    return;
  }

  char subcmd_first = g_parse_buf[0];
  switch (subcmd_first) {
  case 'G': // GET
    flag = parse_next_int(ptr);
    if (flag >= 8) {
      Serial.println(F("ERR: Invalid flag"));
      return;
    }
    Serial.print(F("Flag: "));
    Serial.print(flag);
    Serial.print(F(": "));
    Serial.println(config_get_flag(flag));
    break;
  case 'S': // SET
    flag = parse_next_int(ptr);
    if (flag >= 8) {
      Serial.println(F("ERR: Invalid flag"));
      return;
    }

    value = parse_next_int(ptr);
    if (config_set_flag(flag, value)) {
      Serial.println(F("OK - Flag set"));
    } else {
      Serial.println(F("ERR: Invalid value"));
    }
    break;
  default:
    Serial.println(F("ERR: Usage FLAGS [GET|SET] [FLAG] [VALUE]"));
    break;
  }
}

static void handle_eeprom(const char *args) {
  // ========== OPTIMIZED: Use shared buffer ==========
  const char *ptr = args;

  // Get subcommand
  if (!extract_word(ptr, g_parse_buf, sizeof(g_parse_buf))) {
    Serial.println(F("ERR: Usage EEPROM [READ|WRITE|DUMP]"));
    return;
  }

  char subcmd_first = g_parse_buf[0];
  switch (subcmd_first) {
  case 'R': // READ
    // Read configuration from EEPROM into RAM
    Serial.println(F("Reading configuration from EEPROM..."));
    if (config_load_from_eeprom()) {
      Serial.println(F("OK - Configuration loaded from EEPROM"));
    } else {
      Serial.println(F("ERR: Failed to read from EEPROM"));
    }
    break;

  case 'W': // WRITE
    // Write current RAM configuration to EEPROM
    Serial.println(F("Writing configuration to EEPROM..."));
    if (config_save_to_eeprom()) {
      Serial.println(F("OK - Configuration saved to EEPROM"));
    } else {
      Serial.println(F("ERR: Failed to write to EEPROM"));
    }
    break;

  case 'D': // DUMP
    // Show raw EEPROM contents for debugging
    Serial.println(F("EEPROM Debug Info:"));
    Serial.print(F("  EEPROM Size: "));
    Serial.println(EEPROM_CONFIG_SIZE);
    Serial.print(F("  Config Size: "));
    Serial.println(sizeof(GalvoConfig));
    Serial.print(F("  Start Address: "));
    Serial.println(EEPROM_CONFIG_START);

    Serial.println(F("Raw EEPROM Contents:"));
    for (size_t i = 0; i < EEPROM_CONFIG_SIZE; i++) {
      uint8_t byte_val = eeprom_read_config_byte(i);
      Serial.print(F("  ["));
      Serial.print(i);
      Serial.print(F("]: 0x"));
      if (byte_val < 0x10)
        Serial.print(F("0"));
      Serial.println(byte_val, HEX);
    }
    Serial.println(F("EOC"));
    break;

  default:
    Serial.println(F("ERR: Usage EEPROM [READ|WRITE|DUMP]"));
    break;
  }
}

static void handle_debug(const char *args) {
  // ========== OPTIMIZED: Use shared buffer ==========
  const char *ptr = args;

  // Get subcommand
  bool has_subcmd = extract_word(ptr, g_parse_buf, sizeof(g_parse_buf));

  if (!has_subcmd) {
    Serial.println(F("ERR: Usage DEBUG [UPDATE]"));
    return;
  }

  char subcmd_first = g_parse_buf[0];
  switch (subcmd_first) {
  case 'U': // UPDATE
    debug_update_all();
    Serial.println(F("OK - Debug flags updated"));
    break;
  default:
    Serial.println(F("ERR: Usage DEBUG [UPDATE]"));
    break;
  }
}
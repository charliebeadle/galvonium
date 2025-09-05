#pragma once
#include "../config.h"
#include "../hardware/hardware.h"
#include "StaticSerialCommands.h"
#include <Arduino.h>
#include <avr/wdt.h>

/*
Commands:

help - list all commands with description;
help [CMD] - list subcommands of specific command;

*/

void reset() {
  wdt_disable();
  wdt_enable(WDTO_15MS);
  while (1) {
  }
}

constexpr auto arg_bool = ARG(ArgType::Int, 0, 1, "BOOL");
constexpr auto arg_pin = ARG(ArgType::Int, 3, 9, "pin");
constexpr auto arg_u32 = ARG(ArgType::Int, "uint32");
constexpr auto arg_u16 = ARG(ArgType::Int, 0, 65535, "uint16");
constexpr auto arg_u8 = ARG(ArgType::Int, 0, 255, "uint8");

void cmd_help(SerialCommands &sender, Args &args) {
  sender.getSerial().println(F("Available commands:"));
  sender.listCommands();
}

void cmd_reset(SerialCommands &sender, Args &args) {
  sender.getSerial().println(F("Resetting device..."));
  delay(100);
  reset();
}

void cmd_set_timer_frequency(SerialCommands &sender, Args &args) {
  g_config.timer.frequency = args[0].getInt();
  sender.getSerial().print(F("Timer frequency set to "));
  sender.getSerial().println(g_config.timer.frequency);
}

void cmd_set_timer_enabled(SerialCommands &sender, Args &args) {
  g_config.timer.enabled = args[0].getInt();
  sender.getSerial().print(F("Timer enabled set to "));
  sender.getSerial().println(g_config.timer.enabled);
}
void cmd_enable_timer(SerialCommands &sender, Args &args) {
  g_config.timer.enabled = true;
  sender.getSerial().print(F("Timer enabled set to "));
  sender.getSerial().println(g_config.timer.enabled);
}

void cmd_disable_timer(SerialCommands &sender, Args &args) {
  g_config.timer.enabled = false;
  sender.getSerial().print(F("Timer enabled set to "));
  sender.getSerial().println(g_config.timer.enabled);
}

void cmd_set_dac_flags_a(SerialCommands &sender, Args &args) {
  g_config.dac.dac_flags_a = args[0].getInt();
  sender.getSerial().print(F("DAC flags A set to "));
  sender.getSerial().println(g_config.dac.dac_flags_a);
}
void cmd_set_dac_flags_b(SerialCommands &sender, Args &args) {
  g_config.dac.dac_flags_b = args[0].getInt();
  sender.getSerial().print(F("DAC flags B set to "));
  sender.getSerial().println(g_config.dac.dac_flags_b);
}

void cmd_set_dac_speed(SerialCommands &sender, Args &args) {
  g_config.dac.speed = args[0].getInt();
  sender.getSerial().print(F("DAC speed set to "));
  sender.getSerial().println(g_config.dac.speed);
}
void cmd_set_dac_bit_order(SerialCommands &sender, Args &args) {
  g_config.dac.bit_order = args[0].getInt();
  sender.getSerial().print(F("DAC bit order set to "));
  sender.getSerial().println(g_config.dac.bit_order);
}
void cmd_set_dac_data_mode(SerialCommands &sender, Args &args) {
  g_config.dac.data_mode = args[0].getInt();
  sender.getSerial().print(F("DAC data mode set to "));
  sender.getSerial().println(g_config.dac.data_mode);
}

void cmd_set_laser_pin(SerialCommands &sender, Args &args) {
  g_config.laser.pin = args[0].getInt();
  sender.getSerial().print(F("Laser pin set to "));
  sender.getSerial().println(g_config.laser.pin);
}

void cmd_set_laser_on_dwell(SerialCommands &sender, Args &args) {
  g_config.renderer.laser_on_dwell = args[0].getInt();
  sender.getSerial().print(F("Laser on dwell set to "));
  sender.getSerial().println(g_config.renderer.laser_on_dwell);
}

void cmd_set_laser_off_dwell(SerialCommands &sender, Args &args) {
  g_config.renderer.laser_off_dwell = args[0].getInt();
  sender.getSerial().print(F("Laser off dwell set to "));
  sender.getSerial().println(g_config.renderer.laser_off_dwell);
}

void cmd_set_max_step_size(SerialCommands &sender, Args &args) {
  g_config.renderer.max_step_size = args[0].getInt();
  sender.getSerial().print(F("Max step size set to "));
  sender.getSerial().println(g_config.renderer.max_step_size);
}
void cmd_set_acc_factor(SerialCommands &sender, Args &args) {
  g_config.renderer.acc_factor = args[0].getInt();
  sender.getSerial().print(F("Acc factor set to "));
  sender.getSerial().println(g_config.renderer.acc_factor);
}
void cmd_set_dec_factor(SerialCommands &sender, Args &args) {
  g_config.renderer.dec_factor = args[0].getInt();
  sender.getSerial().print(F("Dec factor set to "));
  sender.getSerial().println(g_config.renderer.dec_factor);
}
void cmd_set_flip_x(SerialCommands &sender, Args &args) {
  g_config.renderer.flip_x = args[0].getInt();
  sender.getSerial().print(F("Flip x set to "));
  sender.getSerial().println(g_config.renderer.flip_x);
}
void cmd_set_flip_y(SerialCommands &sender, Args &args) {
  g_config.renderer.flip_y = args[0].getInt();
  sender.getSerial().print(F("Flip y set to "));
  sender.getSerial().println(g_config.renderer.flip_y);
}
void cmd_set_swap_xy(SerialCommands &sender, Args &args) {
  g_config.renderer.swap_xy = args[0].getInt();
  sender.getSerial().print(F("Swap xy set to "));
  sender.getSerial().println(g_config.renderer.swap_xy);
}

Command set_commands[]{
    COMMAND(cmd_set_timer_frequency, "timer_freq", arg_u32, nullptr,
            "Set the timer frequency"),
    COMMAND(cmd_enable_timer, "enable_timer", nullptr, "Enable the timer"),
    COMMAND(cmd_disable_timer, "disable_timer", nullptr, "Disable the timer"),
    COMMAND(cmd_set_dac_flags_a, "dac_flags_a", arg_u8, nullptr,
            "Set the DAC flags A"),
    COMMAND(cmd_set_dac_flags_b, "dac_flags_b", arg_u8, nullptr,
            "Set the DAC flags B"),
    COMMAND(cmd_set_dac_speed, "dac_speed", arg_u32, nullptr,
            "Set the DAC speed"),
    COMMAND(cmd_set_dac_bit_order, "dac_bit_order", arg_u8, nullptr,
            "Set the DAC bit order"),
    COMMAND(cmd_set_dac_data_mode, "dac_data_mode", arg_u8, nullptr,
            "Set the DAC data mode"),
    COMMAND(cmd_set_laser_pin, "laser_pin", arg_pin, nullptr,
            "Set the laser pin"),
    COMMAND(cmd_set_laser_on_dwell, "laser_on_dwell", arg_u8, nullptr,
            "Set the laser on dwell"),
    COMMAND(cmd_set_laser_off_dwell, "laser_off_dwell", arg_u8, nullptr,
            "Set the laser off dwell"),
    COMMAND(cmd_set_max_step_size, "max_step_size", arg_u8, nullptr,
            "Set the max step size"),
    COMMAND(cmd_set_acc_factor, "acc_factor", arg_u8, nullptr,
            "Set the acc factor"),
    COMMAND(cmd_set_dec_factor, "dec_factor", arg_u8, nullptr,
            "Set the dec factor"),
    COMMAND(cmd_set_flip_x, "flip_x", arg_bool, nullptr, "Set the flip x"),
    COMMAND(cmd_set_flip_y, "flip_y", arg_bool, nullptr, "Set the flip y"),
    COMMAND(cmd_set_swap_xy, "swap_xy", arg_bool, nullptr, "Set the swap xy"),
};

void cmd_set(SerialCommands &sender, Args &args) {
  sender.listAllCommands(set_commands, sizeof(set_commands) / sizeof(Command));
}

void cmd_reload_timer(SerialCommands &sender, Args &args) {
  Hardware::context.update_timer_from_config();
  sender.getSerial().println(F("Timer reloaded"));
}

void cmd_reload_dac(SerialCommands &sender, Args &args) {
  Hardware::context.update_dac_from_config();
  sender.getSerial().println(F("DAC reloaded"));
}
void cmd_reload_laser(SerialCommands &sender, Args &args) {
  Hardware::context.update_laser_from_config();
  sender.getSerial().println(F("Laser reloaded"));
}
void cmd_reload_serial(SerialCommands &sender, Args &args) {
  Hardware::context.update_serial_from_config();
  sender.getSerial().println(F("Serial reloaded"));
}
void cmd_reload_all(SerialCommands &sender, Args &args) {
  Hardware::context.update_all_from_config();
  sender.getSerial().println(F("All reloaded"));
}

Command reload_commands[]{
    COMMAND(cmd_reload_timer, "timer", nullptr, "Reloads the timer"),
    COMMAND(cmd_reload_dac, "dac", nullptr, "Reloads the DAC"),
    COMMAND(cmd_reload_laser, "laser", nullptr, "Reloads the laser"),
    COMMAND(cmd_reload_serial, "serial", nullptr, "Reloads the serial"),
    COMMAND(cmd_reload_all, "all", nullptr, "Reloads all"),
};

void cmd_reload(SerialCommands &sender, Args &args) {
  sender.listAllCommands(reload_commands,
                         sizeof(reload_commands) / sizeof(Command));
}

Command commands[]{
    COMMAND(cmd_help, "help", nullptr, "Prints this help message"),
    COMMAND(cmd_reset, "reset", nullptr, "Resets the device"),
    COMMAND(cmd_set, "set", set_commands, "Sets a parameter"),
    COMMAND(cmd_reload, "reload", reload_commands, "Reloads a parameter"),
};
SerialCommands serialCommands(Serial, commands,
                              sizeof(commands) / sizeof(Command));

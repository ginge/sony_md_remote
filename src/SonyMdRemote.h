/*
 * Sony MD Remote protocol
 * Barry Carter 2022 <barry.carter@gmail.com> 
 *  
 * Do your setup in here
 */
#pragma once
#include "Arduino.h"

// SETUP
// Pin to READ from
#define MD_DATA_PIN      3

// pin to WRITE to
#define MD_SEND_DATA_PIN 4

// serial port, I use USB.
#define MD_SERIAL_PORT   Serial

// save space by disabling certain features
#define MD_ENABLE_RECV   1
#define MD_ENABLE_SEND   1

// TUNING

// How long reset is pulled low for. This is the min it can be
#define RESET_LOW_US_MIN 800
// How long reset is pulled low for. This is the MAX it can be
#define RESET_LOW_US_MAX 1200
// How long does a pulse need to be to be considered "ON"
#define PULSE_WIDTH_ON_US_MIN 100

// Timeout for how long before we consider the packet done.
#define END_MSG_TIMEOUT_US 6500

// When writing to the MD...
// A one  is ----|_|--  (long high, short low)
// A zero is -|____|--  (short high, short low)
//how long a long pulse is as part of the full pulse
#define MD_PULSE_LONG_US    220
// and the low pulse
#define MD_PULSE_SHORT_US   18

#define MD_PULSE_RESET_LOW_US   1100
#define MD_PULSE_RESET_HIGH_US  1000 + MD_PULSE_SHORT_US

// When SENDING, how long between bytes.
#define MD_INTER_BYTE_DELAY     80

// DEBUG
// Dump the raw packet to the USB
#define DUMP_MD_PACKET          1
// verify the bit parity. Disabling can save a few cycles if you are short
#define MD_CALC_RECV_PARITY     1

// PACKET HEADERS
// the packet header starts with these
#define MD_HEADER_0             0x82
#define MD_HEADER_1             0x80
#define MD_HEADER_1a            0x81

// COMMANDS
// the first byte after the address is the command
#define CMD_UNKNOWN_02          0x02
#define CMD_DISP_MODE_MAYBE     0x03
#define CMD_BACKLIGHT           0x05
#define CMD_VOLUME              0x40
#define CMD_PLAY_MODE           0x41
#define CMD_BATTERY             0x43
#define CMD_TRACK               0xA0
#define CMD_PLAY_STATE          0xA1
#define CMD_DISP_MAYBE          0xA2
#define CMD_TEXT                0xC8


// REGISTERS
// some function related specifics

#define REG_TEXT                0x01
#define REG_TEXT_POSITION       0x03
#define REG_TEXT_LEN            0x07

// how big is the text buffer. Your device might not have as much ram as mine...
#define MAX_TEXT_LEN            64

#define CMD_TEXT_APPEND         0x02
#define CMD_TEXT_END            0x01


#define REG_BACKLIGHT           0x01
#define BACKLIGHT_ON            0x7F
#define BACKLIGHT_OFF           0x00


#define REG_VOLUME              0x01


#define REG_BATTERY             0x01
#define BATTERY_CHARGE          0x7F
#define BATTERY_LOW             0x80
#define BATTERY_ZERO            0x01

#define REG_PLAY_MODE           0x01
#define PLAY_MODE_NORMAL        0x00
#define PLAY_MODE_REPEAT        0x01
#define PLAY_MODE_REPEAT_ONE    0x03
#define PLAY_MODE_SHUFFLE       0x7F

#define REG_PLAY_STATE          0x04
#define PLAY_STATE_TOGGLE       0x03
#define PLAY_STATE_ON           0x7F
#define PLAY_STATE_OFF          0x00
#define PLAY_STATE_TRACK        0x01


#define REG_TRACK               0x04


// When in recv mode (emulating a remote), these are some commands we can send
#define MD_RECV_MODE_TEXT       0x82
#define MD_RECV_CRASH           0x83
// nothing 0x84
#define MD_RECV_DISPLAY_TIMER   0x86
 // just payload 1
#define MD_RECV_JUST_PAYLOAD_1  0x87
#define MD_RECV_CRASH_88        0x88
#define MD_RECV_CRASH_8A        0x8A
// This starts to absolutely batter out data. no idea what it is, looback/echo write maybe?
#define MD_RECV_DUMP_8A         0x90
// lazily rotate through the text
#define MD_RECV_MODE_TEXT_SCROLL 0xC0
#define MD_RECV_MODE_DEFAULT    0

// Function defs

void md_setup();
void md_loop();

void md_packet_parse(uint8_t *data);
void md_send_byte(uint8_t byte);
// call this periodically if you have a long string. Check the return status
// @returns complete status. False is not completed sending
bool md_send_text();
void md_set_text(char *newtext);
char *md_get_text();

bool md_get_backlight();
void md_set_backlight(bool isOn);

void md_send_backlight();
void md_set_volume(uint8_t volume);
uint8_t md_get_volume();

void md_send_track();
void md_set_track(uint8_t track);
int md_get_track();
uint8_t md_calculate_parity(uint8_t *data, uint8_t byte_count);
void md_recv_set_send_byte(uint8_t byte_send);

// callback from recv
void md_text_received_cb(char *text, uint8_t len);
void md_packet_just_received_cb(uint8_t *data);

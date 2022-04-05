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
#define RESET_LOW_US_MIN 700
// How long reset is pulled low for. This is the MAX it can be
#define RESET_LOW_US_MAX 1300
// How long does a pulse need to be to be considered "ON"
#define PULSE_WIDTH_ON_US_MIN 100

// Timeout for how long before we consider the packet done.
#define END_MSG_TIMEOUT_US 6500

// Header RW bits for the remote emulation
#define MD_HEADER_REMOTE_READY_FOR_TEXT   1
#define MD_HEADER_REMOTE_TIMER            2
#define MD_HEADER_REMOTE_TX_READY         4
#define MD_HEADER_REMOTE_ERROR            6
#define MD_HEADER_REMOTE_IS_INIT          7

// and the bits for the host emulation.
#define MD_HEADER_HOST_DATA_AVAIL         0
#define MD_HEADER_HOST_BUS_AVAIL          4
#define MD_HEADER_HOST_ERROR              6
#define MD_HEADER_HOST_HOST_READY         7

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

// COMMANDS
// the first byte after the address is the command
#define CMD_CAPABILITIES        0x01
#define CMD_UNKNOWN_02          0x02
#define CMD_DISP_MODE_MAYBE     0x03
#define CMD_BACKLIGHT           0x05
#define CMD_VOLUME              0x40
#define CMD_PLAY_MODE           0x41
#define CMD_REC_MODE            0x42
#define CMD_BATTERY             0x43
#define CMD_EQ                  0x46
#define CMD_ALARM               0x47
#define CMD_TRACK               0xA0
#define CMD_PLAY_STATE          0xA1
#define CMD_DISP_MAYBE          0xA2
#define CMD_TEXT                0xC8

// joint text mode commands
#define CMD_SYNC_GET_ADDR       0x18
#define CMD_SYNC_TRK_CNT        0xD8
#define CMD_SYNC_SET_TRACK      0xD9

// REGISTERS
// some function related specifics

// Text
//===============
#define REG_TEXT                0x01
#define REG_TEXT_POSITION       0x03
#define REG_TEXT_LEN            0x07

// how big is the text buffer. Your device might not have as much ram as mine...
#define MAX_TEXT_LEN            64

#define CMD_TEXT_APPEND         0x02
#define CMD_TEXT_END            0x01

// Capabilities
//===============
#define REG_CAPABILITIES_BLOCK  0x02        

// BACKLIGHT
//===============
#define REG_BACKLIGHT           0x01
#define BACKLIGHT_ON            0x7F
#define BACKLIGHT_OFF           0x00

// Volume
//===============
#define REG_VOLUME              0x01

// Battery
//===============
#define REG_BATTERY             0x01
#define BATTERY_CHARGE          0x7F
#define BATTERY_LOW             0x80
// blink
#define BATTERY_ZERO            0x01
/* bits
[0]  Blink Low Batt
[1]  Power Connected
[2]  Charging
[3] 
[4] 
[5]  Battery Bar bit 0
[6]  Battery Bar bit 1
[7]  Running From Battery
battery bar count is in bit 5 and 6
 i.e. bars = (val >> 5) & (0x7F)
*/

// Play Mode
//===============
#define REG_PLAY_MODE           0x01
#define PLAY_MODE_NORMAL        0x00
#define PLAY_MODE_REPEAT        0x01
#define PLAY_MODE_REPEAT_ONCE   0x02
#define PLAY_MODE_REPEAT_ONE    0x03
#define PLAY_MODE_SHUFFLE       0x04
#define PLAY_MODE_SHUFFLE_REPEAT 0x05
#define PLAY_MODE_PGM           0x06
#define PLAY_MODE_PGM_REPEAT    0x07

// EQ State
//===============
#define EQ_MODE_MORMAL          0x00
#define EQ_MODE_BASS_1          0x01
#define EQ_MODE_BASS_2          0x02
#define EQ_MODE_SOUND_1         0x03
#define EQ_MODE_SOUND_2         0x04

// Play State
//===============
#define REG_PLAY_STATE          0x04
#define PLAY_STATE_TOGGLE       0x03
#define PLAY_STATE_ON           0x7F
#define PLAY_STATE_OFF          0x00
#define PLAY_STATE_TRACK        0x01

// Track Number
//===============
#define REG_SHOW_INDICATOR      0x01
#define REG_TRACK               0x04

// Recording on indicator
//===============
#define REG_RECORDING_INDICATOR 0x01
#define RECORDING_INDICATOR_ENABLED 0x7F

// EQ
//===============
#define REG_EQ                  0x01

// Alarm indicator
//===============
#define REG_ALARM_INDICATOR     0x01
#define ALARM_INDICATOR_ENABLED 0x7F

// Function defs

void md_setup();
void md_loop();

void md_packet_parse(uint8_t *data);
void md_send_byte(uint8_t pin, uint8_t byte);
// call this periodically if you have a long string. Check the return status
// lib
void md_recv_enable(bool is_enabled);

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

bool md_get_alarm_enabled();
void md_set_alarm_enabled();
void md_send_alarm_indicator();

uint8_t md_get_eq();
void md_set_eq(uint8_t eq);
void md_send_eq();

bool md_get_recording_enabled();
void md_set_recording_enabled(bool is_enabled);
void md_send_recording_indicator();

// send display mode now
void md_disp_send_mode();

// lib util
void md_display();

void md_recv_mode_timer();
void md_recv_mode_default();
void md_recv_mode_text();
bool md_is_text_sending();
bool _md_send_text();

void md_request_capabilities(uint8_t block);

// sync without waiting for the next send window. just send now
void md_sync_device();

// recv
void md_recv_setup();
void md_recv_loop();
uint8_t *md_recv_get_send_buf();
void md_recv_set_send_len(uint8_t len);
void md_recv_set_mode(uint8_t mode);
void md_recv_clear_mode(uint8_t mode);
uint8_t md_calculate_parity(uint8_t *data, uint8_t byte_count);
void _poll_pin_change(int level);

// send
void md_send_setup();
void md_send_loop();
uint8_t *md_get_send_buf();
uint8_t md_send_packet(uint8_t *data, uint8_t len);
bool md_send_is_ready_for_text();
bool md_send_is_ready_for_timer();
bool md_send_is_error();
void md_send_data(uint8_t pin, uint8_t *data, uint8_t len, uint8_t wait_pulse);
void _do_send_recv();
uint8_t md_send_get_cmd();

// callback from recv
void md_text_received_cb(char *text, uint8_t len);
void md_packet_just_received_cb(uint8_t *data);


// joint text
void md_jt_sync_device();
void md_jt_start_playback(uint8_t from_track, char *album, char *title);
void md_jt_send_track_break(uint8_t track_id, char *title);

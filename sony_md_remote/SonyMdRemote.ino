/*
 * Sony MD Remote state handler
 * Barry Carter 2022 <barry.carter@gmail.com> 
 *  
 * Processes packets from the MD player into structured data.
 * Has the ability to take updates from the device as well as send data to a remote
 * 
 * The only requirement is you call md_loop() as fast as you can,
 * Use the getters and setters, then _send() to send to a MD device
 * 
 * Example:
 * 
 * void loop() {
 *   md_loop();
 *   md_set_track(99);
 *   md_send_track();
 *   md_set_text("Testing McTest");
 *   md_send_text();
 * }
 */
#include "SonyMdRemote.h"

static void _md_set_battery_raw(uint8_t *data);
static void _md_set_track_raw(uint8_t *data);
static void _md_set_play_state_raw(uint8_t *data);
static void _md_set_disp_raw(uint8_t *data);

// feature vars
char text[MAX_TEXT_LEN];
static uint8_t _cur_text_len = 0;
static uint8_t _text_send_idx = 0;
static bool _send_text = false;;

uint8_t backlight_val = 0;
uint8_t volume_val = 0;
uint8_t play_mode_val = 0;
uint8_t play_state_val = 0;
uint8_t track_val;
uint8_t battery_val;
uint8_t _recv_enabled = 0;

static bool _text_done = false;

void md_packet_parse(uint8_t *data) {
  switch(data[0]) {
    case CMD_CAPABILITIES:
      _md_capabilities(data);
    case CMD_TEXT:
      _md_set_text_raw(data);
      break;
    case CMD_BACKLIGHT:
      _md_set_backlight_raw(data);
      break;
    case CMD_VOLUME:
      _md_set_volume_raw(data);
      break;
    case CMD_PLAY_MODE:
      _md_set_play_mode_raw(data);
      break;
    case CMD_BATTERY:
      _md_set_battery_raw(data);
      break;
    case CMD_TRACK:
      _md_set_track_raw(data);
      break;
    case CMD_PLAY_STATE:
      _md_set_play_state_raw(data);
      break;
    case CMD_DISP_MAYBE:
      _md_set_disp_raw(data);
      break;
    default:
      break;
  }
}

void md_recv_enable(uint8_t is_enabled) {
  _recv_enabled = is_enabled;
}

void _md_capabilities(uint8_t *data) {
  uint8_t *buf = md_recv_get_send_buf();

#define MD_CAPABILILTY_REMOTE          0xC0
#define MD_CAPABILILTY_BLOCK           0x01
// ( 0x9)
#define MD_CAPABILILTY_CHARS           0xFF
#define MD_CAPABILILTY_UNK3            0x00
#define MD_CAPABILILTY_UNK4            0x00
#define MD_CAPABILILTY_UNK5            0x0F
#define MD_CAPABILILTY_HEIGHT_PX       6
#define MD_CAPABILILTY_WIDTH_PX        12
#define MD_CAPABILILTY_CHARSET         0x80
#define MD_CAPABILILTY_UNK9            0x23

/*block 2
 0xc0, 0x2, 0x9, 0x1, 0x8, 0x1, 0x67, 0x0, 0x0, 0x0, 
block 6
0x92, 0x90, 0x83, 0x4d, 0x44, 0x20, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8a,
*/
  // fill the buffer with capabilities
  buf[0] = MD_CAPABILILTY_REMOTE;
  buf[1] = MD_CAPABILILTY_BLOCK;
  buf[2] = MD_CAPABILILTY_CHARS;
  buf[3] = MD_CAPABILILTY_UNK3;
  buf[4] = MD_CAPABILILTY_UNK4;
  buf[5] = MD_CAPABILILTY_UNK5;
  buf[6] = MD_CAPABILILTY_HEIGHT_PX;
  buf[7] = MD_CAPABILILTY_WIDTH_PX;
  buf[8] = MD_CAPABILILTY_CHARSET;
  buf[9] = MD_CAPABILILTY_UNK9;
  
  // set the buffer size
  md_recv_set_send_len(10);
  // this will get sent automatically now
}

char *md_get_text() {
  return text;
}

void md_set_text(char *newtext) {
  strcpy(text, newtext);
  _cur_text_len = strlen(newtext) + 1;
}

bool md_send_text() {
  //_send_text = true;
  // send the first block, the rest goes async
  //_md_send_text();
}

bool _md_send_text() {
  uint8_t *send_buf = md_get_send_buf();
  uint8_t sub_cmd = CMD_TEXT_APPEND;
  
  // TODO
  // We need to send 0x03 to start scrolling if the text is long enough. Usually after a couple of segments
  //uint8_t cmd = md_send_get_cmd();
    
  send_buf[0] = CMD_TEXT;
  send_buf[2] = 0x00;

  // less than 7 bytes of text left. flag as done
  if ((_cur_text_len - _text_send_idx) <= 7)
    sub_cmd = CMD_TEXT_END;
    
  for(int i = 0; i < 7; i++) {
    if (text[_text_send_idx] == 0)
      send_buf[REG_TEXT_POSITION + i] = 0xFF;
    else
      send_buf[REG_TEXT_POSITION + i] = text[_text_send_idx];
    
    _text_send_idx++;

    if (_text_send_idx >= _cur_text_len) {
      _send_text = false;
      if (i == 7)
        send_buf[REG_TEXT_POSITION + i] = 0xFF;
      else
        send_buf[REG_TEXT_POSITION + i] = ' ';
    }
  }
  if (!_send_text)
    _text_send_idx = 0;
  send_buf[REG_TEXT] = sub_cmd;
  
  uint8_t v = md_send_packet(send_buf, 10);
  Serial.println("T");

  // send our completeness status
  return _text_send_idx == 0;
}

static void _md_reset_text() {
  _cur_text_len = 0;
  text[0] = 0;
  _text_send_idx = 0;
}

static void _md_set_text_raw(uint8_t *data) {
  uint8_t reg = data[REG_TEXT];
  // the next time we get some text, check to see if we need to reset the buffer instead of appending
  if (_text_done) {
    _text_done = false;
    _cur_text_len = 0;
  }

  // set the mode to more text please
  md_recv_set_mode(MD_HEADER_REMOTE_READY_FOR_TEXT);
  
  // clear the request time mode bit when we get any text
  md_recv_clear_mode(MD_HEADER_REMOTE_TIMER);
  
  // just keep appending text
  for(int i = 0; i < REG_TEXT_LEN; i++) {
    // 0xFF is the end of text signal
    if (data[i + REG_TEXT_POSITION] == 0xFF) {
      text[_cur_text_len] = 0;
      _text_done = true;
    } else {  
      text[_cur_text_len] = (char)(data[i + REG_TEXT_POSITION]);
    }

    _cur_text_len++;
    
    // don't overflow the text buffer
    if (_cur_text_len > MAX_TEXT_LEN)
      _cur_text_len = 0;

    // null term it
    text[_cur_text_len] = 0;
  }
  
  // last chunk of text received
  if (reg == CMD_TEXT_END) {
    md_recv_clear_mode(MD_HEADER_REMOTE_READY_FOR_TEXT);
    md_text_received_cb(text, (uint8_t)_cur_text_len);
    _text_done = true;
  }  
}

void md_set_backlight(bool isOn) {
  if (isOn)
    backlight_val = BACKLIGHT_ON;
  else
    backlight_val = BACKLIGHT_OFF;
}

bool md_get_backlight() {
  return backlight_val == BACKLIGHT_ON;
}

static void _md_set_backlight_raw(uint8_t *data) {
  backlight_val = data[REG_BACKLIGHT];
}

void md_send_backlight() {
  uint8_t *send_buf = md_get_send_buf();
  send_buf[0] = CMD_BACKLIGHT;
  send_buf[REG_BACKLIGHT] = backlight_val;
  md_send_packet(send_buf, 10);
}

void md_set_volume(uint8_t volume) {
  volume_val = volume;
}

uint8_t md_get_volume() {
  return volume_val;
}

void _md_set_volume_raw(uint8_t *data) {
  volume_val = data[REG_VOLUME];
}

       
bool md_get_play_mode_repeat() {
  return play_mode_val == PLAY_MODE_REPEAT;
}

bool md_get_play_mode_repeat_one() {
  return play_mode_val == PLAY_MODE_REPEAT_ONE;
}

bool md_get_play_mode_shuffle() {
  return play_mode_val == PLAY_MODE_SHUFFLE;
}

void _md_set_play_mode_raw(uint8_t *data) {
  play_mode_val = data[REG_PLAY_MODE];
}

bool md_battery_is_charging() {
  return battery_val == BATTERY_CHARGE;
}

bool md_battery_is_low() {
  return battery_val == BATTERY_LOW;
}

uint8_t get_battery_level() {
  if (battery_val == BATTERY_CHARGE)
    return 0;
  else if (battery_val == BATTERY_LOW)
    return 0;
  else if (battery_val == BATTERY_ZERO)
    return 0;
  else {
    uint8_t chg = battery_val >> 5;
    chg &= 0xFB;
    return chg + 1;
  }
}
            
void _md_set_battery_raw(uint8_t *data) {
  battery_val = data[REG_BATTERY];  
}

int md_get_track() {
  return ((track_val >> 4) * 10) + (track_val & 0xF);
}

void md_set_track(uint8_t track) {
  track_val = ((track / 10) << 4) | track % 10;
  //_md_reset_text();
}

void md_send_track() {
  uint8_t *send_buf = md_get_send_buf();
  send_buf[0] = CMD_TRACK;
  send_buf[REG_TRACK] = track_val;
  md_send_packet(send_buf, 10);
}

static void _md_set_track_raw(uint8_t *data) {
  uint8_t reg = data[REG_TRACK];
  if (track_val != reg) {
    // track changed
    _md_reset_text();
  }
  track_val = reg;
}

uint8_t md_get_play_state() {
  return play_state_val;
}

void _md_set_play_state_raw(uint8_t *data) {
  play_state_val = data[REG_PLAY_STATE];
}

void _md_set_disp_raw(uint8_t *data) {
  //_md_reset_text();
}


void md_recv_mode_text() {
  md_recv_set_mode(MD_HEADER_REMOTE_READY_FOR_TEXT);
}

/*
void md_recv_mode_scroll_text() {
  md_recv_set_send_byte(MD_RECV_MODE_TEXT_SCROLL);
}
*/

void md_recv_mode_default() {
    md_recv_set_mode(MD_HEADER_REMOTE_READY_FOR_TEXT);
}

void md_recv_mode_timer() {
    md_recv_set_mode(MD_HEADER_REMOTE_TIMER);
}

void md_display() {
  uint8_t track = md_get_track();
  char bat[10];

  if (md_battery_is_charging())
    sprintf(bat, "CHG");
  else if (md_battery_is_low())
    sprintf(bat, "LOW");
  else
    sprintf(bat, "%d", get_battery_level());
    
  MD_SERIAL_PORT.printf("[%2d] %-64s %d [R: %d R1: %d S: %d] [B: %s] \n", 
    track, 
    md_get_text(), 
    md_get_play_mode_repeat(),
    md_get_play_mode_repeat_one(),
    md_get_play_mode_shuffle(),
    md_get_play_state(),
    bat);
}

void md_setup() {
#if MD_ENABLE_RECV
  md_recv_setup();
#endif
#if MD_ENABLE_SEND
  md_send_setup();
#endif
md_recv_set_mode(MD_HEADER_REMOTE_READY_FOR_TEXT);
}

void md_loop() {
#if MD_ENABLE_RECV
  if (_recv_enabled)
    md_recv_loop();
#endif
  // todo
  // deal with text here
  // if send text flag set
  if(/*_send_text &&*/ md_send_ready_for_text()) {
    _send_text = true;
    _md_send_text();
  }
  //md_send_loop();
}

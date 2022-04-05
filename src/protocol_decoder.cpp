/*
 * Sony MD Remote Receiver
 * Barry Carter 2022 <barry.carter@gmail.com> 
 *  
 *  Read packets from the MD player in polling mode.
 *  
 *  handles r/w to the device etc
 * 
 */
#include "sony_md_remote.h"
#if MD_ENABLE_RECV

static uint8_t _prev_level;
static uint8_t _state;
static unsigned long _recv_started;
static unsigned long _pulse_duration;
static unsigned long _recv_ended;
static uint8_t _byte_buf[30];
static uint8_t _byte_idx;
#if DUMP_MD_PACKET
static String _tmp_prnt_buf;
#endif
static uint8_t _md_recv_send_byte = 0;
static uint8_t _md_recv_send_buf[10];
static uint8_t _md_recv_send_len = 0;

static void _md_process_start();
static void _decode_md_protocol();
void _poll_pin_change(int level);
static void _gather_packets();
static void _md_recv_send_packet();

void md_recv_set_mode(uint8_t mode) {
  _md_recv_send_byte |= 1 << mode;
}

void md_recv_clear_mode(uint8_t mode) {
  _md_recv_send_byte &= ~(1 << mode);
}

enum MdDecode_state {
  _stateWaitingForStart,
  _stateResetLow,
  _statePackets,
};

// Given a positive pulse/sync start, get us to the end of the start message
// and then gather the bytes up
static void _md_process_start() {
  // reset pulse
  _state = _statePackets;
  // skip past the start bit
  _poll_pin_change(LOW);
  _poll_pin_change(HIGH);
  _poll_pin_change(LOW);
  _poll_pin_change(HIGH);
  
  _prev_level = 1;
  // if we have data to send, flag we want to send it.
  if (_md_recv_send_len) {
    md_recv_set_mode(MD_HEADER_REMOTE_TX_READY);
    Serial.println("TXR");
  }
  else 
    md_recv_clear_mode(MD_HEADER_REMOTE_TX_READY);

  _gather_packets();
}

// Sit and wait for a reset pulse. If there is no reset, this just returns
// If we get a reset, the packets are gathered into the buffer
static void _decode_md_protocol() {
  if (_state == _stateResetLow) {
    _md_process_start();
    return;
  }
  _state = _stateWaitingForStart;
  _gather_packets();
}

// sit and wait for a pin to toggle
void _poll_pin_change(int level) {
  while(digitalReadFast(MD_DATA_PIN) == level);;
}

// get all of the bits and bytes for a full packet
static void _gather_packets() {
  volatile uint8_t tmp_data = 0;
  volatile uint8_t _bit_counter = 0;
  _byte_idx = 0;
  
  while(1) {
    if (_state != _statePackets && _state != _stateWaitingForStart) {
      break;
    }
    
    int level = digitalRead(MD_DATA_PIN);

    // move along
    if (_prev_level == level)
      continue;
  
    _prev_level = level;    
    _recv_ended = micros();
  
    // get the _pulse_duration
    _pulse_duration = _recv_ended - _recv_started;
    _recv_started = _recv_ended;

    // hmm we got a reset while harvesting bits
    if (level == 1 
        && _pulse_duration > RESET_LOW_US_MIN 
        && _pulse_duration < RESET_LOW_US_MAX) {
      _state = _stateResetLow;
      if (_bit_counter > 3) {
        Serial.print("RST! ");
        Serial.println(_bit_counter);
      }
      continue;
    }

    // During the first "bit" we can set to write mode and send
    // some modal data
    if (_state == _statePackets
         && level == 0 
         && _md_recv_send_byte 
         && _byte_idx == 0) {
      if (_md_recv_send_byte & (1 << _bit_counter)) {
        pinMode(MD_DATA_PIN, OUTPUT);
        digitalWrite(MD_DATA_PIN, HIGH);
        delayMicroseconds(MD_PULSE_LONG_US);
        pinMode(MD_DATA_PIN, INPUT);
      }
    }
    
    // set the bit if the high pulse is long
    if (level == 1 && _pulse_duration < PULSE_WIDTH_ON_US_MIN) {
        tmp_data |= (1 << _bit_counter);
    }

    if (level == 1)
      _bit_counter++;

    // we got a whole byte, bank it
    if (_bit_counter >= 8) {
      _bit_counter = 0;
      _byte_buf[_byte_idx] = tmp_data;      
      _byte_idx++;
      
      // Stop when we have a full 10 byte packet (plus header and parity)
      if (_byte_idx >= 13)
        _state = _stateWaitingForStart;

      tmp_data = 0;
    }
  }
}

static void _md_recv_send_packet() {  
  // wait for pulse 0 before each send of a byte
  md_send_data(MD_DATA_PIN, _md_recv_send_buf, _md_recv_send_len, 1);
  _md_recv_send_len = 0;
}

// given a 10 byte data array, crunch the parity
uint8_t md_calculate_parity(uint8_t *data, uint8_t byte_count) {
  uint8_t parity = 0;
  for(int i = 0; i < byte_count; i++) {
    parity = parity ^ data[i];
  }
  return parity;
}

uint8_t *md_recv_get_send_buf() {
  memset(_md_recv_send_buf, 0, 10);
  return _md_recv_send_buf;
}

void md_recv_set_send_len(uint8_t len) {
  _md_recv_send_len = len;
}

void md_recv_setup() {
  pinMode(MD_DATA_PIN, INPUT);
  // tell the md we are ready
  md_recv_set_mode(MD_HEADER_REMOTE_IS_INIT);
}

void md_recv_loop()
{
  int parity = 0;
  _decode_md_protocol();
  if (_byte_idx > 0) {
    // Check the header is valid:
    if (!(_byte_buf[1] & (1 << MD_HEADER_HOST_HOST_READY))) {
      //MD_SERIAL_PORT.println("Host not ready");
      return;
    }
    
    if (_byte_buf[1] & (1 << MD_HEADER_HOST_BUS_AVAIL)) {
      if (_md_recv_send_len) {
        _md_recv_send_packet();
      }
      return;
    }

    // The device continually sends just a header, like a sync I guess
    if (_byte_idx <= 2) {
      //MD_SERIAL_PORT.println("NOP");
      return;
    }

#if DUMP_MD_PACKET
    for(int i = 0; i < _byte_idx; i++) {
      _tmp_prnt_buf.concat(_byte_buf[i]);
      _tmp_prnt_buf.concat(",");
    }
    MD_SERIAL_PORT.println(_tmp_prnt_buf);
    _tmp_prnt_buf = "!";
#endif

#if MD_CALC_RECV_PARITY
    parity = md_calculate_parity(&_byte_buf[2], 10);
    if (_byte_buf[12] != parity) {
      MD_SERIAL_PORT.print("Bad parity ");
      MD_SERIAL_PORT.println(parity);
      return;
    }
#endif

    // callback
    md_packet_just_received_cb(&_byte_buf[2]);
    // parse the packet data
    md_packet_parse(&_byte_buf[2]);
    _byte_idx = 0;
  }
}

void __attribute__((weak)) md_packet_just_received_cb(uint8_t *data) {}
void __attribute__((weak)) md_text_received_cb(char *text, uint8_t len) {}
#endif

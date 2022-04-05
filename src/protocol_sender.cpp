/*
 * Sony MD Remote Sender
 * Barry Carter 2022 <barry.carter@gmail.com> 
 *  
 *  Send data to the minidisc remote control
 *  
 *  Can also read the state/cmd back from the MD remote 
 */
#include "sony_md_remote.h"
#if MD_ENABLE_SEND

static IntervalTimer _read_timer;
static uint8_t _send_buffer[10];
static uint8_t _cmd;
static uint8_t _send_cmd;
unsigned long lastsend;

uint8_t md_send_read_byte();
void _set_data_available(bool is_avail);
void _set_bus_available(bool is_avail);

static void _md_send_reset() {
  digitalWrite(MD_SEND_DATA_PIN, LOW);
  delayMicroseconds(MD_PULSE_RESET_LOW_US);
  digitalWrite(MD_SEND_DATA_PIN, HIGH);
  delayMicroseconds(MD_PULSE_RESET_HIGH_US);
}

static void _md_send_one(uint8_t pin) {
  delayMicroseconds(MD_PULSE_SHORT_US);
  digitalWrite(pin, HIGH);
  delayMicroseconds(MD_PULSE_LONG_US);
  digitalWrite(pin, LOW);
  delayMicroseconds(MD_PULSE_SHORT_US);
  digitalWrite(pin, HIGH);
}

static void _md_send_zero(uint8_t pin) {
  delayMicroseconds(MD_PULSE_SHORT_US);
  digitalWrite(pin, LOW);
  delayMicroseconds(MD_PULSE_LONG_US);
  digitalWrite(pin, HIGH);
}

static uint8_t _md_send_header(bool allowed_to_read) {
  // here, we actually need to go tri-state and capture what the remote and wants
  uint8_t rw_byte = md_send_read_byte();

  _send_cmd |= (1 << MD_HEADER_HOST_HOST_READY);
  //Serial.printf("rw %d %d\n", rw_byte, _send_cmd);

  if (allowed_to_read && (rw_byte & (1 << MD_HEADER_REMOTE_TX_READY))) {
    _set_bus_available(true);
    _set_data_available(true);
  }

  md_send_byte(MD_SEND_DATA_PIN, _send_cmd);

  return rw_byte;
}


uint8_t md_send_read_byte() {
  uint8_t data = 0;
  uint8_t d = 0;
  // we are controlling the clock, so in between each clock pulse, 
  // go into read mode, get the value. then delay for a pulse before clocking high again
  for(int i = 0; i < 8; i++) {
    uint8_t  pinstate = 0;
    delayMicroseconds(MD_PULSE_SHORT_US);
    // go to read mode
    pinMode(MD_SEND_DATA_PIN, INPUT);
    delayMicroseconds(20);

    delayMicroseconds(MD_PULSE_LONG_US - 20);
    pinstate = digitalRead(MD_SEND_DATA_PIN);
            
    // back to output mode
    if (pinstate) {
        pinMode(MD_SEND_DATA_PIN, OUTPUT);
        digitalWrite(MD_SEND_DATA_PIN, LOW);
        delayMicroseconds(MD_PULSE_SHORT_US);
        digitalWrite(MD_SEND_DATA_PIN, HIGH);   
    } else {
        delayMicroseconds(MD_PULSE_SHORT_US);
        pinMode(MD_SEND_DATA_PIN, OUTPUT);
        digitalWrite(MD_SEND_DATA_PIN, HIGH);
    }    
    
    data |= (pinstate << i);
  }
  
  return data;
}

void md_send_byte(uint8_t pin, uint8_t data_byte) {
  for(int i = 0; i < 8; i++) {
    if (data_byte & 1 << i)
      _md_send_one(pin);
    else
      _md_send_zero(pin);
  }
  delayMicroseconds(MD_INTER_BYTE_DELAY);
}

void md_send_data(uint8_t pin, uint8_t *data, uint8_t len, uint8_t wait_pulse) {
  for(int i = 0; i < len; i++) {
    if (wait_pulse) {
      _poll_pin_change(LOW);
      _poll_pin_change(HIGH);
    }
    md_send_byte(pin, data[i]);
    delayMicroseconds(MD_INTER_BYTE_DELAY);
  }
  md_send_byte(pin, md_calculate_parity(data, len));
}

void _set_data_available(bool is_avail) {
  // it's inverted
  if (is_avail)
    _send_cmd &= ~(1 << MD_HEADER_HOST_DATA_AVAIL);
  else
    _send_cmd |= (1 << MD_HEADER_HOST_DATA_AVAIL);
}


void _set_bus_available(bool is_avail) {
  if (is_avail)
    _send_cmd |= (1 << MD_HEADER_HOST_BUS_AVAIL);
  else
    _send_cmd &= ~(1 << MD_HEADER_HOST_BUS_AVAIL);    
}

uint8_t md_send_packet(uint8_t *data, uint8_t len) {
  _md_send_reset();
  _md_send_zero(MD_SEND_DATA_PIN);
  _set_data_available(true);
  _set_bus_available(false);
  uint8_t cmd = _md_send_header(false);

  // could add a callback here to allow the host app to determine payload if it wants to
  // for now, the cmd is returned. let the sender deal with cmd modes
  md_send_data(MD_SEND_DATA_PIN, data, len, 0);  

  //// dont set it for now
  _cmd = cmd;
  Serial.println(_cmd);
  lastsend = micros();
  return cmd;
}

void md_send_setup() {
  pinMode(MD_SEND_DATA_PIN, OUTPUT);
  // start with the data pin active high
  digitalWrite(MD_SEND_DATA_PIN, HIGH);
  // wait for the line to settle
  delayMicroseconds(8000);
}

bool _read_packet() {
  //delayMicroseconds(10);
  // read 10 bytes + parity
  for(int i = 0; i < 10; i++) {
    delayMicroseconds(MD_PULSE_LONG_US);
    _md_send_zero(MD_SEND_DATA_PIN);

    uint8_t a = md_send_read_byte();
    _send_buffer[i] = a;
    Serial.printf("%02x", a);
    Serial.print(" ");
  }
  delayMicroseconds(MD_PULSE_LONG_US);
  _md_send_zero(MD_SEND_DATA_PIN);
  
  Serial.println(md_calculate_parity(_send_buffer, 10));
  bool done = md_calculate_parity(_send_buffer, 10) == md_send_read_byte();

}

// do a NOP right now, unless we have some data to recieve, in which case read it in
void _do_send_recv() {
  lastsend = micros();
  _md_send_reset();
  _md_send_zero(MD_SEND_DATA_PIN);
  _set_bus_available(false);
  _set_data_available(false);
  _cmd = _md_send_header(true);
  //Serial.println(_cmd);
  if (_cmd & (1 << MD_HEADER_REMOTE_TX_READY)) {
    Serial.println("S:RECV");
    // read in the data
    _read_packet();
    _set_bus_available(true);
  }
}

// call me periodically!
void md_send_loop() {
  unsigned long tnow = micros();
  bool send_now = false;//_cmd & (1 << MD_HEADER_REMOTE_TX_READY);
  
  // if time has elapsed, send a nop
  if (send_now || tnow - lastsend > 32000) {      
    _do_send_recv();
  }
}

// get the tx buffer so the app can send some stuff
uint8_t *md_get_send_buf() {
  memset(_send_buffer, 0, 10);
  return _send_buffer;
}

// check to see if we are ready for text
bool md_send_is_ready_for_text() {
  uint8_t a = (_cmd & 1 << MD_HEADER_REMOTE_READY_FOR_TEXT);
  _cmd &= ~(1 << MD_HEADER_REMOTE_READY_FOR_TEXT);
  return a;
}

bool md_send_is_ready_for_timer() {
  uint8_t a = (_cmd & 1 << MD_HEADER_REMOTE_TIMER);
  _cmd &= ~(1 << MD_HEADER_REMOTE_TIMER);
  return a;
}

bool md_send_is_error() {
  return (_cmd & (1 << MD_HEADER_REMOTE_ERROR));
}

uint8_t md_send_get_cmd() {
  return _cmd;
}
#endif

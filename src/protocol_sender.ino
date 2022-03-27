/*
 * Sony MD Remote Sender
 * Barry Carter 2022 <barry.carter@gmail.com> 
 *  
 *  Send data to the minidisc remote control
 *  
 *  Can also read the state/cmd back from the MD remote 
 */
#if MD_ENABLE_SEND
static IntervalTimer _read_timer;
static uint8_t _send_buffer[10];

static void _md_send_reset() {
  digitalWrite(MD_SEND_DATA_PIN, LOW);
  delayMicroseconds(MD_PULSE_RESET_LOW_US);
  digitalWrite(MD_SEND_DATA_PIN, HIGH);
  delayMicroseconds(MD_PULSE_RESET_HIGH_US);
}

static void _md_send_one() {
  delayMicroseconds(MD_PULSE_SHORT_US);
  digitalWrite(MD_SEND_DATA_PIN, HIGH);
  delayMicroseconds(MD_PULSE_LONG_US);
  digitalWrite(MD_SEND_DATA_PIN, LOW);
  delayMicroseconds(MD_PULSE_SHORT_US);
  digitalWrite(MD_SEND_DATA_PIN, HIGH);
}

static void _md_send_zero() {
  delayMicroseconds(MD_PULSE_SHORT_US);
  digitalWrite(MD_SEND_DATA_PIN, LOW);
  delayMicroseconds(MD_PULSE_LONG_US);
  digitalWrite(MD_SEND_DATA_PIN, HIGH);
}

static uint8_t _md_send_header() {
  // here, we actually need to go tri-state and capture what the remote and wants
  uint8_t rw_byte = md_send_read_byte();
  md_send_byte(MD_HEADER_1);

  return rw_byte;
}


uint8_t md_send_read_byte() {
  uint8_t data = 0;
  
  for(int i = 0; i < 8; i++) {
    uint8_t  pinstate = 0;
    
    // go to read mode    
    pinMode(MD_SEND_DATA_PIN, INPUT);
    delayMicroseconds(10);
    pinstate = digitalRead(MD_SEND_DATA_PIN);
    
    // back to output mode
    digitalWrite(MD_SEND_DATA_PIN, LOW);
    pinMode(MD_SEND_DATA_PIN, OUTPUT);
    delayMicroseconds(MD_PULSE_SHORT_US - 10);
    digitalWrite(MD_SEND_DATA_PIN, LOW);
    delayMicroseconds(MD_PULSE_LONG_US);
    digitalWrite(MD_SEND_DATA_PIN, LOW);
    delayMicroseconds(MD_PULSE_SHORT_US);
    digitalWrite(MD_SEND_DATA_PIN, HIGH);
    //delayMicroseconds(MD_PULSE_SHORT_US);
    data |= (pinstate << i);
  }
  delayMicroseconds(MD_INTER_BYTE_DELAY);
  
  return data;
}

void md_send_byte(uint8_t data_byte) {
  for(int i = 0; i < 8; i++) {
    if (data_byte & 1 << i)
      _md_send_one();
    else
      _md_send_zero();
  }
  delayMicroseconds(MD_INTER_BYTE_DELAY);
}

uint8_t md_send_packet(uint8_t *data, uint8_t len) {
  _md_send_reset();
  _md_send_zero();
  uint8_t cmd = _md_send_header();

  // could add a callback here to allow the host app to determine payload if it wants to
  // for now, the cmd is returned. let the sender deal with cmd modes

  for(int i = 0; i < len; i++) {
    md_send_byte(data[i]);
  }
  md_send_byte(md_calculate_parity(data, len));

  return cmd;
}

void md_send_setup() {
  pinMode(MD_SEND_DATA_PIN, OUTPUT);
  // start with the data pin active high
  digitalWrite(MD_SEND_DATA_PIN, HIGH);
}

// call me periodically. I might be doing something in the future
void md_send_loop() {
 // send a "nop" ?
}

// get the tx buffer so the app can send some stuff
uint8_t *md_get_send_buf() {
  memset(_send_buffer, 0, 10);
  return _send_buffer;
}
#endif

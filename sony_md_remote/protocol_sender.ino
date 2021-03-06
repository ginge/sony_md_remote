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
static uint8_t _cmd;
static uint8_t _send_cmd;
unsigned long lastsend;

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

static uint8_t _md_send_header() {
  // here, we actually need to go tri-state and capture what the remote and wants
  uint8_t rw_byte = md_send_read_byte();

  _send_cmd |= (1 << MD_HEADER_HOST_HOST_READY);

  md_send_byte(MD_SEND_DATA_PIN, _send_cmd);

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

uint8_t md_send_packet(uint8_t *data, uint8_t len) {
  _md_send_reset();
  _md_send_zero(MD_SEND_DATA_PIN);
  _set_data_available(true);
  uint8_t cmd = _md_send_header();

  // could add a callback here to allow the host app to determine payload if it wants to
  // for now, the cmd is returned. let the sender deal with cmd modes
  md_send_data(MD_SEND_DATA_PIN, data, len, 0);  

  // dont set it for now
  //_cmd = cmd;
  lastsend = micros();
  return cmd;
}

void md_send_setup() {
  pinMode(MD_SEND_DATA_PIN, OUTPUT);
  // start with the data pin active high
  digitalWrite(MD_SEND_DATA_PIN, HIGH);
}


// call me periodically!
void md_send_loop() {
  unsigned long tnow = micros();

  // if time has elapsed, send a nop
  if (tnow - lastsend > 32000) {
    lastsend = micros();
     _md_send_reset();
    _md_send_zero(MD_SEND_DATA_PIN);
    _set_data_available(false);
    _cmd = _md_send_header();
    Serial.println(_cmd);
    if (_cmd & (1 << MD_HEADER_REMOTE_TX_READY)) {
      //md_recv_data();
      Serial.println("S:RECV");
    }
    if (_cmd & (1 << MD_HEADER_REMOTE_READY_FOR_TEXT)) {
      
      Serial.println("S:TXT");
    }
  }
 // send a "nop" ?
}

// get the tx buffer so the app can send some stuff
uint8_t *md_get_send_buf() {
  memset(_send_buffer, 0, 10);
  return _send_buffer;
}

bool md_send_ready_for_text() {
  uint8_t a = (_cmd & 1 << MD_HEADER_REMOTE_READY_FOR_TEXT);
  _cmd &= ~(1 << MD_HEADER_REMOTE_READY_FOR_TEXT);
  return a;
}

uint8_t md_send_get_cmd() {
  return _cmd;
}
#endif

/*
 * Sony MD Remote Receiver
 * Barry Carter 2022 <barry.carter@gmail.com> 
 *  
 *  Read packets from the MD player in polling mode.
 *  
 *  handles r/w to the device etc
 * 
 */
#if MD_ENABLE_RECV
static IntervalTimer _packet_end_timeout_timer;
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
  _gather_packets();
}

// Sit and wait for a reset pulse. If there is no reset, this just returns
// If we get a reset, the packets are gathered into the buffer
static void _decode_md_protocol() {
  if (_state == _stateResetLow) {
    if (_pulse_duration < 700)
      MD_SERIAL_PORT.println(_pulse_duration);
    _md_process_start();
    return;
  }
  _state = _stateWaitingForStart;
  _gather_packets();
  //return;
}

// sit and wait for a pin to toggle
static void _poll_pin_change(int level) {
  while(digitalReadFast(MD_DATA_PIN) == level);;
}

// get all of the bits and bytes for a full packet
static void _gather_packets() {
  volatile uint8_t tmp_data = 0;
  volatile uint8_t _bit_counter = 0;
  _byte_idx = 0;
  
  while(1) {
    if (_state != _statePackets && _state != _stateWaitingForStart) {
      _packet_end_timeout_timer.end();
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

      // set the timer going
      _packet_end_timeout_timer.begin(md_check_timeout, END_MSG_TIMEOUT_US);
    }
  }
}

// triggered from a timer. Once this is triggered, we are done with the processing of packets
void md_check_timeout() {
  if (_state == _statePackets)
    _state = _stateWaitingForStart;
}

// given a 10 byte data array, crunch the parity
uint8_t md_calculate_parity(uint8_t *data, uint8_t byte_count) {
  uint8_t parity = 0;
  for(int i = 0; i < byte_count; i++) {
    parity = parity ^ data[i];
  }
  return parity;
}

void md_recv_setup() {
  pinMode(MD_DATA_PIN, INPUT);
}

void md_recv_loop()
{
  int parity = 0;
  _decode_md_protocol();
  if (_byte_idx > 0) {
    // Check the header is valid:
    if (_byte_buf[1] != MD_HEADER_1 && _byte_buf[1] != MD_HEADER_1a) { // || _byte_buf[1] != 0x81) {
      MD_SERIAL_PORT.print("Invalid Header ");
      MD_SERIAL_PORT.print(_byte_buf[0]);
      MD_SERIAL_PORT.print(" ");
      MD_SERIAL_PORT.println(_byte_buf[1]);
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

void md_recv_set_send_byte(uint8_t byte_send) {
  _md_recv_send_byte = byte_send;
}
#endif

/*
 * Generic protocol raw dumper
 * Barry Carter 2022 <barry.carter@gmail.com> 
 *  
 * Read from a single pin and dump the data
 * 
 * Capture each pulse, encode it as text and send it to the host for decoding
 * Not massively useful on its own, but will create a basic csv to parse
 */

// we make an assumption that the protocol is going to be delimited
#define END_MSG_TIMEOUT_US 6500
#define TRACE_DATA_PIN     3

String buf = "";

volatile unsigned long startTime = 0;

void setup() {
  Serial.begin(115200);
  pinMode(TRACE_DATA_PIN, INPUT);
  Serial.println("MD Raw"); 
}


void waitLevel(boolean level) {
  startTime = micros();

  while (digitalReadFast(TRACE_DATA_PIN) == level) {
  }

  unsigned long pulseLen = micros() - startTime;

  // set the line levels as "-PREVIOUS_DURATION,+PREVIOUS...
  // csv delimited
  if (level == LOW) {
    buf.concat("-");
  } else if (level == HIGH) {
    buf.concat("+");
  }

  buf.concat(pulseLen);
  buf.concat(",");

  // we got a timeout between packets. Send to the host
  if (pulseLen > END_MSG_TIMEOUT_US) {
    Serial.println(buf);
    // start the message with a # so the host can process it
    buf = "#";
  }
}

// just wait for the pulses to come in. When we have a long pulse, send it to the host
void loop() {
  waitLevel(LOW);
  waitLevel(HIGH);
}

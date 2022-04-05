#include "src/sony_md_remote.h"

/*
 * Sony MD Remote example
 * Barry Carter 2022 <barry.carter@gmail.com> 
 *  
 * Read from the MD player, and display it as best we understand
 * 
 * This basic example only displays the md interface through USB, 
 * gets the text and then switches to time
 * 
 */

void setup() {
  Serial.begin(115200);
  Serial.println("MD test");
  md_setup();

  while (1) {
    md_loop();
    if (md_send_get_cmd() == 128 || md_send_get_cmd() == 160)
      break;
    delayMicroseconds(30000);
  }

  while (1) {
    md_loop();
    delayMicroseconds(40000);
    md_jt_sync_device();
    if (!md_send_is_error())
      break;
  }
  Serial.println("Synced");
  md_jt_start_playback(1, " ", "  ");
}

char incoming_buffer[128];
uint8_t buffer_idx;
uint8_t incoming_byte;
unsigned long time_last;

char album[128];

void process_buffer() {
  
  // set album name
  if (incoming_buffer[0] == 0x01) {
    memcpy(album, &incoming_buffer[1], 126);
    Serial.println(album);
  }
  // Start playback
  else if (incoming_buffer[0] == 0x05) {
    md_jt_start_playback(1, album, &incoming_buffer[1]);
    Serial.println(&incoming_buffer[1]);
  } 
  // send track
  else if (incoming_buffer[0] == 0x06) {
    md_jt_send_track_break(2, &incoming_buffer[1]);
    Serial.println(&incoming_buffer[1]);
  }
  
}

void loop() {
  md_loop();

  unsigned long time_now = millis();

  if (time_now - time_last > 1000)
    buffer_idx = 0;

  if (Serial.available() > 0) {
    incoming_byte = Serial.read(); // read the incoming byte:
    incoming_buffer[buffer_idx] = incoming_byte;
    buffer_idx++;
    time_last = time_now;
    if (buffer_idx > 127) {
      Serial.println("Too Long!");
      buffer_idx = 0;
      return;
    }

    // cr
    if (buffer_idx > 1 && (incoming_byte == 10 || incoming_byte == 13)) {
      Serial.println("Got!");
      incoming_buffer[buffer_idx - 1] = 0; // null term
      process_buffer();
      buffer_idx = 0;
    }
  }  
}

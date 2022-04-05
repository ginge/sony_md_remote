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
  md_setup();
  // Turn on the receiver
  md_recv_enable(1);
  Serial.begin(115200);
  Serial.println("MD test");  
}

void loop() {
  // not much to do here, just keep calling the md function
  md_loop();
}

// Example callbacks from the md player
// we can do some processing when we get a packet, or decode some text.
void md_packet_just_received_cb(uint8_t *packet) {
  // Dump a fake display to serial for debugging purposes
  md_display();
}

// when the final text chunk is received, set the display back to timer
void md_text_received_cb(char *text, uint8_t len) {
  // The MD remote displays the last block of text for a couple of seconds, then requests timer mode
  // once we get a ' ', it's likely the timer, so now we switch off the request
  if (text[0] != ' ')
    md_recv_mode_timer();
  
  // just keep sending me text
  // the real remote requests text for 2 ish display chunks, then waits until one has scrolled out of view
  // once there is only one text block left, it requests text again.
  md_recv_mode_text();
}

#include "SonyMdRemote.h"

/*
 * Sony MD Remote example
 * Barry Carter 2022 <barry.carter@gmail.com> 
 *  
 * Read from the MD player, and replay it as best we understand back to the device. 
 * Not just a 1:1 relay, but real parsing, understanding, reconstruction of the data
 * 
 * This basic example only displays the md interface through USB, 
 * gets the text and then switches to time
 * 
 */
void setup() {
  md_setup();
  Serial.begin(115200);
  Serial.println("MD test");
  md_set_track(99);
  md_set_backlight(true);
}

unsigned long t;

void loop() {
/*  md_set_text((char *)"Here is some text long text");

  unsigned long tnow = millis();
  if (tnow - t > 10000) {
    t = tnow;
  
  // Turn the backlight on, say hi and then go into a recv loop
  md_set_backlight(true);
  md_send_backlight();
  
  md_set_text((char *)"Hello");
  md_send_text();

  delayMicroseconds(32000);
  md_send_track();
  delayMicroseconds(32000);
  delay(1000);
  md_set_text((char *)"How");
  md_send_text();

  delay(1000);
  md_set_text((char *)"Are");
  md_send_text();
  delay(1000);
  md_set_text((char *)"You?");
  md_send_text();
  delay(1000);
  md_set_backlight(false);
  md_send_backlight();
  delay(1000);

 
  //md_send_text();

  }
  md_loop();
return;*/
  // drop into read mode
  md_recv_enable(1);
  while(1)
    md_loop();
}

// Example callbacks from the md player
// we can do some processing when we get a packet, or decode some text.
int cnt = 0;

void md_packet_just_received_cb(uint8_t *packet) {

/* An example of sending sequential packets in between reading
  if (cnt == 0)
    md_send_backlight();
  else if (cnt == 1)
    md_send_track();
  else if (cnt == 2)
    md_send_text();
  
  cnt++;
  if (cnt == 4)
    cnt = 0;
    */

  //md_send_text();
  //md_send_track();

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

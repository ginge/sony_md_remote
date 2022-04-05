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
unsigned long last_stamp = 0;
char buf[20];
bool timer_mode = false;
bool display_timer = false;
bool do_update = false;

typedef struct disc_track  {
  char title[40];
  int length_s;
} disc_track;

typedef struct disc  {
  char album_name[20];
  int tracks;
  disc_track track[3];
} disc;


disc music = {
  "Test Album",
  3,
  {
    { "Paradise is Minidisc - Some Guy",  46, },
    { "Where did it all go? - Doomsayer",  68, },
    { "I'm a Minidisc Player! - One1!",  42, },
  }
};

int current_track;
unsigned long timer;

void setup() {
  md_setup();
  Serial.begin(115200);
  Serial.println("MD test");

  while (1) {
    md_loop();
    if (md_send_get_cmd() == 128)
      break;
    delayMicroseconds(30000);
  }

  for(int i = 0; i < 20; i++) {
    md_loop();
    delayMicroseconds(40000);
  }

  //while (1) {
    md_loop();
    delayMicroseconds(40000);
    //md_jt_start_playback(1, (char *)"test", (char *)"title");
    md_jt_send_track_break(1, (char *)"Titleb");
    Serial.println(md_send_get_cmd());
    //if (!md_send_is_error())
    //  break;
    delayMicroseconds(40000);
  //}
  while(1) {
    md_loop();
    delayMicroseconds(40000);
  }
  return;
  
  //md_set_text((char*)"some long text to set");
  md_set_text(music.track[0].title);
  md_set_track(1);
  md_send_track();
  md_disp_send_mode();
  delayMicroseconds(30000);

    
  digitalWrite(MD_SEND_DATA_PIN, LOW);
  delayMicroseconds(200);
  digitalWrite(MD_SEND_DATA_PIN, HIGH);

  Serial.println("CAP 1");
  delayMicroseconds(30000);
  md_request_capabilities(1);
  delayMicroseconds(30000);
  _do_send_recv();

Serial.println("CAP 2");
  delayMicroseconds(30000);
  md_request_capabilities(2);
  delayMicroseconds(30000);
  _do_send_recv();
  Serial.println("CAP 3");
  delayMicroseconds(30000);
  md_request_capabilities(3);
  delayMicroseconds(30000);
  _do_send_recv();
  Serial.println("CAP 4");
  delayMicroseconds(30000);
  md_request_capabilities(4);
  delayMicroseconds(30000);
  _do_send_recv();
  Serial.println("CAP 5");
  delayMicroseconds(30000);
  char *buf = md_get_send_buf();

  md_request_capabilities(5);
  delayMicroseconds(30000);
  _do_send_recv();
  Serial.printf("CAP 5 [SERIAL]: %s\n", &buf[2]);
  delayMicroseconds(30000);

  Serial.println("CAP 6");
  md_request_capabilities(6);
  delayMicroseconds(30000);
  _do_send_recv();
  Serial.println("CAP 7");
  delayMicroseconds(30000);
  
  md_request_capabilities(7);
  delayMicroseconds(30000);
  _do_send_recv();
  Serial.println("CAP 8");
    delayMicroseconds(30000);
  
  md_request_capabilities(8);
  delayMicroseconds(30000);
  _do_send_recv();
  Serial.println("CAP 9");
    delayMicroseconds(30000);
  
  md_request_capabilities(9);
  delayMicroseconds(30000);
  _do_send_recv();
Serial.println("CAP 10");
  md_request_capabilities(10);
  delayMicroseconds(30000);
  _do_send_recv();
  
}

uint8_t tracknum;

void loop() {
  
  // not much to do here, just keep calling the md function
  md_loop();
  unsigned long tnow = millis();

  if (tnow - last_stamp > 10000) {
    Serial.println("SEND");
    md_jt_start_playback(1, (char *)"test", (char *)"title");
    md_jt_send_track_break(tracknum, (char *)"Titlea");
    last_stamp = tnow;
    tracknum++;
    return;
  }
  /*if (tnow - last_stamp > 13000) {
    Serial.println("GO NOW");
    last_stamp = tnow;
    return;
  }
  if (tnow - last_stamp > 10000) {
    Serial.println("PAUSE NOW");
  }*/
  //last_stamp = tnow;

  return;
  // at some point the display will ask for timer mode...
  timer_mode = md_send_is_ready_for_timer();
  if (timer_mode) {
    display_timer = true;
    Serial.print(tnow - last_stamp);
    Serial.println("TMR");
    timer_mode = false;
    // trigger a send of the data
    do_update = true;
  }

  if (tnow - last_stamp > 1000) {
    timer += 1000;
    do_update = true;
  }

  if (do_update) {
    do_update = false;
    if (timer / 1000 > music.track[current_track].length_s) {
      display_timer = false;
      current_track++;
      md_disp_send_mode();
      md_set_text(music.track[current_track].title);
      timer = 0;
    }

    md_set_track(current_track + 1);
    md_send_track();

    if (display_timer) {
      unsigned long seconds = timer / 1000;
      unsigned long minutes = seconds / 60;
      unsigned long hours = minutes / 60;
      
      seconds %= 60;
      minutes %= 60;
      hours %= 24;
      sprintf(buf, " %02d:%02d", (int)minutes, (int)seconds);
      md_set_text(buf);
    }
    last_stamp = tnow;
    
    if (current_track >= 2) {
      current_track = 0;
    }
  }
  
}

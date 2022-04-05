/*
 * Sony MD Remote Joint Text handler
 * Barry Carter 2022 <barry.carter@gmail.com> 
 *  
 * Joint text titler for recording from an optical source to MD
 * Allows the track breaks to be added and titles to be transferred to the MD
 */
// Joint text mode when transferring audio from a CD to a minidisc
// This uses a cable to transfer the data from 
// Usually this needs a RK-TXT01 link cable (just can make one)
// and a qualifying minidisc recorder
// It uses host send mode to send bits to the md recorder's special pin
// The init sequence is as follows:
//  WRITE 0xD9 SYNC R/W packet
//  READ  an address from the MD host
//  WRITE the same address back to the MD host
// Once playback is started, the MD recording, we send
//  WRITE 0xD8 Track count info
//  WRITE 0xD9 Set Track with specific fields filled
//  WRITE 0xD9 zeros
//  WRITE 0xC8 send album text as usual host protocol
//  WRITE 0xD9 Set current track id only
//  WRITE 0xC8 send title text as usual host protocol
// For each new track
//  WRITE Init sequence
//  WRITE 0xD9 Set current track id only
//  WRITE 0xC8 send title text as usual host protocol
#include "sony_md_remote.h"

void md_jt_send_init_data();
void md_jt_send_album(char *album);
void md_jt_send_text(char *text);

// start initial handshake
void md_jt_sync_device() {
  // 0x18 - get device write address
  // this will return us a new address to write to
  uint8_t *send_buf = md_get_send_buf();
  send_buf[0] = CMD_SYNC_GET_ADDR;
  md_send_packet(send_buf, 10);
  
  uint8_t cmd = md_send_get_cmd();
  if (md_send_is_error()) {
      return;
  }
  
  delayMicroseconds(30000);
  // read: address: data
  _do_send_recv();
  delayMicroseconds(30000);

  // write to the address the same data we just read
  md_send_packet(send_buf, 10);
  delayMicroseconds(30000);
  
  // read [0] value
  _do_send_recv();
  delayMicroseconds(30000);
  // write 0xd8 0x00 0x11 0x01
  memset(send_buf, 0, 10);
  send_buf[0] = CMD_SYNC_TRK_CNT;
  send_buf[1] = 0x00;
  send_buf[2] = 0x11;
  send_buf[3] = 0x01; // current track?
  md_send_packet(send_buf, 10);
  delayMicroseconds(30000);
}

void md_jt_send_init_data() {
  // set next track id, send text
  //0xd9 0x00 [0x00 0x1c] < set track number
  uint8_t *send_buf = md_get_send_buf();
  send_buf[0] = CMD_SYNC_SET_TRACK;
  send_buf[1] = 0x01;
  send_buf[2] = 0x01;
  send_buf[3] = 0x01;  // current track
  send_buf[4] = 0x1C;  // total tracks on source media
  send_buf[5] = 0x61;  // track length in s?
  send_buf[6] = 0x18;
  send_buf[7] = 0x00;
  send_buf[8] = 0xFF;
  send_buf[9] = 0x00; 
  md_send_packet(send_buf, 10);
  delayMicroseconds(30000);  
}

void md_jt_send_album(char *album) {
  // send all 0's
  //_do_send_recv();
  uint8_t *send_buf = md_get_send_buf();
  send_buf[0] = CMD_SYNC_SET_TRACK;
  delayMicroseconds(30000);
  md_send_packet(send_buf, 10);
  delayMicroseconds(30000);
  md_jt_send_text(album);
  delayMicroseconds(30000);
}

void  md_jt_send_text(char *text) {
  uint8_t *send_buf = md_get_send_buf();
  send_buf[0] = CMD_TEXT;
  send_buf[1] = 2;
  send_buf[2] = 1;
  int i = 0;
  for  (int a = 0; a < strlen(text); a++) {
    if (a % 7 ==0 && a > 0) {
      md_send_packet(send_buf, 10);
      memset(send_buf, 0, sizeof(send_buf));
      delayMicroseconds(30000);
      i = 0;
    }
    send_buf[REG_TEXT_POSITION + i] = text[a];
    Serial.print(" ");
    Serial.print(send_buf[REG_TEXT_POSITION + i], HEX);
    
    i++;
  }
  delayMicroseconds(30000);
  md_send_packet(send_buf, 10);


  // send a last block of just zeros
  delayMicroseconds(30000);
  send_buf = md_get_send_buf();
  send_buf[0] = CMD_TEXT;
  send_buf[1] = 1;
  send_buf[2] = 1;
  md_send_packet(send_buf, 10);
  delayMicroseconds(30000);
  _do_send_recv();
}

void md_jt_send_track_break(uint8_t track_id, char *title) {
  uint8_t *send_buf = md_get_send_buf();
  send_buf[0] = CMD_SYNC_SET_TRACK;
  
  send_buf[3] = track_id; // it's probaby using more than one byte, but not observed yet
  md_send_packet(send_buf, 10);
  delayMicroseconds(30000);
  md_jt_send_text(title);
  
  // after a while, we get a read request and the payload
  // [0x15, 0x00, ... 0x00]
}

// send start_playback, then periodically send track breaks after each has played
void md_jt_start_playback(uint8_t from_track, char *album, char *title) {
  delayMicroseconds(35000);
  md_jt_send_init_data();
  delayMicroseconds(35000);
  md_jt_send_album(album);
      delayMicroseconds(35000);

  uint8_t *send_buf = md_get_send_buf();
  send_buf[0] = CMD_SYNC_SET_TRACK;  
  send_buf[3] = from_track; // it's probaby using more than one byte, but not observed yet
  md_send_packet(send_buf, 10);
  delayMicroseconds(35000);
  md_jt_send_text(title);
  _do_send_recv();
}

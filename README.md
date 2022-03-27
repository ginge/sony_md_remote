# Sony Minidisc Remote Control Client and Host Library

Barry Carter 2022 <barry.carter@gmail.com>

## What is it?
A library to emulate being a Sony remote control display AND controlling an existing remote control display. 
A library that lets you control the remote control display of the Sony range of devices.
Additionally, the library supports emulation of a remote control
Specifically tested are the Minidsc MZR-90/91/55 devices.


### Use Cases?
Who knows, my use case is kinda niche. 1) I was curious about the protocol, I might not have a use for this. 2) I May want to integrate this into a cyberdeck device in the future.
Here are some other ideas:
 * Build a slick little display for an existing Sony device
 * Intercept the display text and track details, change them on the fly and create hilarity with your friends! (okay, this is a stretch)
 * get a bluetooth uart and make a mobile phone display worthy of yesteryear! Who needs a smart watch!
 
## Display Protocol

![Protocol](sony_md_protocol.png)

### Wire Protocol
The protcol is surprisingly sort of bi directional, in that the client device (the remote control)  can plant a byte of data into the host device for processing. This allows the remote control to request different display modes, text and a couple of other noted features.
 
The protocol is carried out on a single wire interface (plus power +, -)
The single pin is responsible for sync, tri-state handshaking and data payload.
 
The protcol itself is quite basic, at least in basic mode....

8 bits per byte, active low
 
### Basic mode:
 [Sync][High][Start Bit][Data Byte 0*][Data Byte 1]..... [optional 10 byte Payload]
 
 * The first byte is special, as the protocol goes into a tri-state, i.e. the host becomes an input.
 However it does this only between clock pulses. There is more on this later

You can live life entirely in this mode if you don't need text

### Not so basic 
"Did I just move from classical physics to the quantum realm?"
 
When you realise the protocol is actually bidirectional, there is simultaneous elation and immediate dismay. What can of worms did I just open?
Well friends, here is the basis for the Sony Remote Quantun realm.
 
During the first byte of the header, after each clocked LOW pulse, the host goes into input mode.
This allows you to write 1 byte of data to the host while it is clocking.
(achieved in software with some speedy toggling from output to input, ala i2c)
 
``` The handshake sequence is like this:
 Host Mode:     [O][ ][ ][I]
 Host Level:    [H][L][ ][ ]
 Remote Mode:   [I][ ][O][ ]
 Remote Level:  [ ][ ][ ][H]
 
 O = output, I = Input, H = High, L = Low
```
The host device continually writes a header byte, this presumably serves 2 purposes:
 1) sync, i'm still here
 2) opportunity for the client remote to ask for data from the host device
 
### What do you write?
So far I have observed commands to:
 * Request first/next block of text (if the track is long, you can only get it in 7 byte increments). Once you have all of the text, no more is sent and the display command not updated
 * Display of track number in text
 * Request the text display in a loop forever
 * crash the minidisc player (bonus feature?)
   
This could be quite a useful feaure to control...
If you are running out of processing time, you can request the text, stop requesting while you process the first block, request the next chunk etc.

### Communication Protocol
The procol itself is really simple. All little endian, nothing to twiddle.
After the header, the protocol is formatted as follows:

[Command Byte][[Data Payload][Parity CRC]

Some commands have a subcommand, such as text.

### Commands
| ID   | Purpose     | Notes                         |
| -----|-------------|-------------------------------|
| 0x03 | Disp Clear? | Sets display mode             |
| 0x04 |             |                               |
| 0x05 | Backlight   |   0x7F = on                   |
| 0x40 | Volume      |                               |
| 0x41 | Play Mode   |                               |
| 0x43 | Battery     |                               |
| 0xA0 | Track       |                               |
| 0xA1 | Play State  |                               |
| 0xA2 | Display Mode|                               |
| 0xC8 | Text        |                               |

#### Volume
current volume: when data[0] == 64, display current volume. Volume = data[2]

volume change: when data[0] <= 30, volume = data [0]

#### Track
4 bit encoded. Low bits are units, high bits tens

#### Play Mode
see .h file for more, but contains shuffle, repeat, 1, etc modes

#### Text
Subcommand: data[1]

Text Start: data[3]

Sent in chunks of 7 characters

Subcommand 
* = 2 means more text coming
* = 1 means last chuck of text


#### Parity
The usual running XOR affair. Nothing special here. Move along.

## "Library"
There are three parts to this arduino sketch
1) A protocol receiver and decoder. This will listen to the desired pin, wait for a packet and then process it. Once processed, a callback is called with the 10 bytes of the packet (assuming it passes parity crc)
2) A protocol sender. This lets you fill raw data into the 10 byte buffer, and send it off to the device. Quite raw.
3) a class library to wrap the two above scripts together. The library processes the incoming packets into plain variables "volume, track" etc. Additionally you can call _send(); functions to send the stored variable data to the minidisc remote control display.

The library can be used with or without sending or receiving. Typically I would imagine you will only one to do one of the two functions at a time.

In pure send more, you can manually set values, i.e. "md_set_track(50);" and then immediately send them to the display using "md_send_track();"

### Notes:
* The library blocks as it polls or sends. This might not leave you much processing time.
* The library will only start processing once the start bit is detected.
* There are callbacks for processing each packet. This is a good place to to per-packet processing  or custom updates. Not the best place for heavy logic
* This library is a while weekend of effort. i.e. it's pretty crap and un-optimised.

 
## TODO
 * Track needs hundreds adding
 * more understanding about the tri-state read/write mode
 * add remaining send routines
 * add remaining recv routines?
 * Finish prototype code to press buttons on the MD player using analogWrite
 * I suppose I should add the read compliment to the above
 
## How did you figure out the protocol?
(for my memory)

With an oscilloscope, a Teensy, and a pot of coffee.

First I looked at it on the scope. Pretty simple.

Then I wrote two programs:
 1) MdRawDump. This script times the high low pulses from the MD and writes them to serial. Really basic, really quick
 2) MdParser.py  This reads the pulses, reconstructs them into data packets and parses them into known data. Similar to the library above!
 
Once I had a realtime pc tracking app, I could iterate on the data coming through. poke a button, observe, rinse repeat. Drink coffee, swear, ponder and code.

Figuring out the bi-directional nature to get the text was a little more challenging and required a reasoned try it and see approach. The theory that something was going on with that first byte paid out. The first byte had a sloping fall time to it, not a nice sharp pulse like you would expect. It looked a lot like it went open drain.... A few moments later and some terrible hacky code, the text was flowing!

# Credits
Way too late into this project I found this repo: 
 * https://github.com/xunker/SonyMDRemote by Matthew Nielsen. A great resouce that backs up a lot of my observations and adds some things to my TODO list. (like, really I totally forgot track count could be > 99... derp)
 While totally awesome, some information here superceeds it (although not as amazingly documented as Matthew did)
 * Ringtons Coffee. Damn good stuff and get me through the weekend of research.
 * Big credits to the oscope for leading me to the conclusion this was a bidirectional protocol. A few resistors hanging off the board really highlighted that open drain slope like a digital circuit could (and did) not.

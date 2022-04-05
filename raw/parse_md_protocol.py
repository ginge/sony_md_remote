import sys
import glob
import serial
import datetime



def serial_ports():
    """ Lists serial port names

        :raises EnvironmentError:
            On unsupported or unknown platforms
        :returns:
            A list of the serial ports available on the system
    """
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/ttyA[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

ports = serial_ports()
print(f"Using Port: {ports[0]}")
ser = serial.Serial(ports[0])
ser.flushInput()

timeout = 5000
reset_low = 800
pulse_high = -100


CMD_UNKNOWN_02         = 0x02
CMD_DISP_MODE_MAYBE    = 0x03
CMD_BACKLIGHT          = 0x05
CMD_VOLUME             = 0x40
CMD_PLAY_MODE          = 0x41
CMD_BATTERY            = 0x43
CMD_TRACK              = 0xA0
CMD_PLAY_STATE         = 0xA1
CMD_DISP_MAYBE         = 0xA2
CMD_TEXT               = 0xC8

REG_BATTERY            = 0x03
BATTERY_CHARGE         = 0x7F
BATTERY_LOW            = 0x80
BATTERY_ZERO           = 0x01

REG_PLAY_MODE          = 0x03
PLAY_MODE_NORMAL       = 0x00
PLAY_MODE_REPEAT       = 0x01
PLAY_MODE_REPEAT_ONE   = 0x03
PLAY_MODE_SHUFFLE      = 0x7F

REG_PLAY_STATE         = 0x06
CMD_PLAY_STATE_TOGGLE  = 0x03
CMD_PLAY_STATE_ON      = 0x7F
CMD_PLAY_STATE_OFF     = 0x00
CMD_PLAY_STATE_TRACK   = 0x01

REG_BACKLIGHT          = 0x03
CMD_BACKLIGHT_ON       = 0x7F
CMD_BACKLIGHT_OFF      = 0x00

REG_VOLUME             = 0x03

REG_TEXT               = 0x03
REG_TEXT_POSITION      = 0x05
REG_TEXT_LEN           = 0x07

CMD_TEXT_APPEND        = 0x02
CMD_TEXT_END           = 0x01

REG_TRACK              = 0x06

class SonyMdRemote:
    def __init__(self):
        self.Backlight = 0
        self.Text = ""
        self.Track = 0
        self.PlayState = 0
        self.PlayMode = 0
        self.Battery = 0
        self.Volume = 0
    
        self.TextBuf = ""
        
    
    
    def process_packet(self, data):
        # ignore printing NOPs
        
        str = ""
        for d in data:
            str += hex(d) + ", "
        print(f"[{datetime.datetime.now().strftime('%H:%M:%S.%f')}] Packet: [ {str} ]")

        if len(data) <= 2:
            return
        
        # check header values 82 80
        if data[1] != 0x80:
            print("BAD: Invalid packet!")
                
        cmd = data[2]
        
        # when display mode is pressed this is triggered
        if cmd == CMD_UNKNOWN_02:
            a = 1        
        elif cmd == CMD_DISP_MAYBE:
            a = 1        
        elif cmd == CMD_BACKLIGHT:
            md.set_backlight_raw(data)       
        elif cmd == CMD_VOLUME:
            md.set_volume_raw(data)
        elif cmd == CMD_TEXT:
            md.set_text_raw(data)       
        elif cmd == CMD_TRACK:
            md.set_track_raw(data)        
        elif cmd == CMD_PLAY_STATE:
            md.set_play_state_raw(data)    
        elif cmd == CMD_PLAY_MODE:
            md.set_play_mode_raw(data)            
        elif cmd == CMD_BATTERY:
            md.set_battery_raw(data)
        else:
            str = ""
            for d in data:
                str += hex(d) + ", "
            print(f"[{datetime.datetime.now().strftime('%H:%M:%S.%f')}] Unknown Packet: [ {str} ]")
        
    def is_backlight_on(self):
        return self.Backlight == CMD_BACKLIGHT_ON
    
    def set_backlight(self, isOn):
        self.Backlight = CMD_BACKLIGHT_ON
        
    def set_backlight_raw(self, data):
        bl = data[REG_BACKLIGHT]
        self.Backlight = bl
        print(f"Backlight {self.Backlight == CMD_BACKLIGHT_ON}")
        
    def get_volume(self):
        return self.Volume
    
    def set_volume(self, val):
        self.Volume = val
    
    def set_volume_raw(self, data):
        vol = data[REG_VOLUME]
        self.Volume = vol
        print (f"Volume {self.Volume}")
        
    def set_text_raw(self, data):
        reg = data[REG_TEXT]
        # just keep appending text
        if self.TextBuf == "":
            self.Text = ""
        self.Text += ''.join(chr(e) for e in data[REG_TEXT_POSITION:REG_TEXT_POSITION + REG_TEXT_LEN])
        self.TextBuf = self.Text
        
        # last chunk of text received
        if reg == CMD_TEXT_END:
            self.Text = self.Text.replace(chr(0xff), chr(0x00))
            print(f"{self.Text}")
            self.TextBuf = ""
        
    def get_track(self):
        return self.Track
    
    def set_track(self, track):
        self.Track = track
        self.Text = ""

    def set_track_raw(self, data):
        reg = data[REG_TRACK]
        if self.Track != reg:
            self.set_track(reg)
        self.Track = reg
        print (f"Track T {self.Track}")
    
    def get_play_state(self):
        return self.PlayState
    
    def set_play_state_raw(self, data):
        reg = data[REG_PLAY_STATE]
        self.PlayState = reg
        if reg == CMD_PLAY_STATE_TOGGLE:
            print (f"Disc: TOGGLE")
        elif reg == CMD_PLAY_STATE_ON:
            print (f"Disc: ALL ON")
        elif reg == CMD_PLAY_STATE_OFF:
            print (f"Disc: ALL OFF")
        elif reg == CMD_PLAY_STATE_TRACK:
            print (f"Disc: TRACK CHANGE")
            
    def get_play_mode_repeat(self):
        return self.PlayMode == PLAY_MODE_REPEAT
    
    def get_play_mode_repeat_one(self):
        return self.PlayMode == PLAY_MODE_REPEAT_ONE
    
    def get_play_mode_shuffle(self):
        return self.PlayMode == PLAY_MODE_SHUFFLE
        
    def set_play_mode_raw(self, data):
        reg = data[REG_PLAY_MODE]
        self.PlayMode = reg
        if reg == PLAY_MODE_NORMAL:
            print("Play Mode: No Suffle/repeat")
        elif reg == PLAY_MODE_REPEAT:
            print("Play Mode: Repeat")
        elif reg == PLAY_MODE_REPEAT_ONE:
            print("Play Mode: Repeat ONE")
        elif reg == PLAY_MODE_SHUFFLE:
            print("Play Mode: SHUFFLE")
        
    def get_battery_is_charging(self):
        return self.Battery == BATTERY_CHARGE
    
    def get_battery_is_low(self):
        return self.Battery == BATTERY_LOW
        
    def get_battery_level(self):
        reg = self.Battery
        if reg == BATTERY_CHARGE:
            return 0
        elif reg == BATTERY_LOW:
            return 0
        elif reg == BATTERY_ZERO:
            return 0
        else:
            chg = reg >> 5
            chg = set_bit(chg, 2, 0)
            return chg + 1
        
    def set_battery_raw(self, data):
        reg = data[REG_BATTERY]
        print(f"BAT {reg}")
        if  reg == BATTERY_CHARGE:
            print (f"Battery: Charge ({reg})")
        elif reg == BATTERY_LOW:
            print (f"Battery: Low Battery")
        elif reg == BATTERY_ZERO:
            print (f"Battery: 0 Bars!")
        else:
            chg = reg >> 5
            chg = set_bit(chg, 2, 0)
            print (f"Battery: Charge ({chg + 1} Bars)")
        self.Battery = reg
        
    def display(self):
        disp = "".ljust(10, ' ')
        if self.get_play_mode_repeat():
            disp = "Repeat".ljust(10, ' ')
        elif self.get_play_mode_repeat_one():
            disp = "Repeat One".ljust(10, ' ')
        elif self.get_play_mode_shuffle():
            disp = "Shuffle".ljust(10, ' ')
        
        x = self.get_play_state()
        bat = self.get_battery_is_charging()
        batT = "CHG"
        if bat == 0:
            bat = self.get_battery_is_low()
            batT = "LOW"
        if bat == 0:
            bat = self.get_battery_level()
            batT = ("O" * bat).ljust(4, ' ')
                    
        print(f"[{self.Track}] {self.Text.ljust(20, ' ')} {x} [Repeat: {disp}] [B:{batT}]")
        
def set_bit(v, index, x):
    """Set the index:th bit of v to 1 if x is truthy, else to 0, and return the new value."""
    mask = 1 << index   # Compute mask, an integer with just bit 'index' set.
    v &= ~mask          # Clear the bit indicated by the mask (if x is False)
    if x:
        v |= mask         # If x was True, set the bit indicated by the mask.
    return v            # Return the result, we're done.

def flip_byte(c):
  c = ((c >> 1) & 0x55) | ((c << 1) & 0xAA)
  c = ((c >> 2) & 0x33) | ((c << 2) & 0xCC)
  c = (c >> 4) | (c << 4)

  return c


md = SonyMdRemote()

def process_packet(packet_array):
    global md
    
    i = 0
    j = 0
    outp = 0
    addr = 0
    data = []
    bit_counter = 0
    state = 0
    skp = 0
    
    #print(packet_array)
    for p in packet_array:
        # skip the sync and start bit
        if i < 4:
            i = i + 1
            state = 1 # packet detect
            continue

        if state == 1: # packet detect
            # if we detect writes, skip the start pulse
            if len(data) > 1 and data[0] == 0x92 and bit_counter == 0 and skp < 2:
                i = i + 1
                skp = skp + 1
                continue
            
            # gather the bits
            if not bit_counter % 2:
                #print(p)
                if p < 0 and p < pulse_high:
                    outp = set_bit(outp, int(bit_counter/2), 0)
                else:
                    outp = set_bit(outp, int(bit_counter/2), 1)
            
            bit_counter = bit_counter + 1
            
            if bit_counter >= 16:
                #outp = outp & 0xFE
                #print(f"gfhdhgf {outp}")
                data.append(outp)
                bit_counter = 0
                state = 1 # detect stop
                skp = 0
                continue       
        

        i = i + 1
    
    md.process_packet(data)

buf = ""
num_buf = ""
packet_bits = []

def process_bytes(ser_bytes):
    buf = ser_bytes.replace("!", "")
    
    spl = buf.split(',')
    ispl = []
    for v in range(len(spl)):
        if spl[v] == '':
            continue
        ispl.append(int(spl[v]))

    md.process_packet(ispl)
    md.display()

while True:
    try:
        ser_bytes = ser.readline()
        #print(ser_bytes)
        
        buf = str(ser_bytes.decode("utf-8"))
        buf = buf.replace("\n", "")
        buf = buf.replace("\r", "")
        buf = buf.replace("--", "-")
        num = 0
        state = 0
        packet_val = 0
        
        if buf[0] == '!':            
            process_bytes(buf)
            continue
                
        if buf[0] != '#':
            continue
        
        for b in range(len(buf)):
            data = buf[b]
            
            if data == '+':
                num_buf = ""
                continue
            
            if data == '-':
                num_buf = ""
            
            if data == ',':
                if num_buf == "" or num_buf == "-" or num_buf == "+":
                    continue
                num = int(num_buf)
                #print(num_buf)
                num_buf = ""
                packet_bits.append(num)
                if num > timeout:
                    process_packet(packet_bits)
                    packet_bits.clear()
                    #print("Start Packet")
                    md.display()
                    continue

                num_buf = ""
                continue
                
            num_buf += data

    except KeyboardInterrupt:
        print("Bye")
        ser.close()
        sys.exit()
    except Exception:
        print("error")
        print(buf)
        print(traceback)
        break
    
    

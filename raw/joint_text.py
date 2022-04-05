import sys
import glob
import serial
import datetime
import time
from subprocess import Popen


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

string = "Album Name"
data = bytes(string, "ascii")
ser.write(bytes([1]))
ser.write(data)
ser.write('\r'.encode())

p = Popen(['mpg123', '../test_audio/1.mp3'])
#time.sleep(0.1)
# write the start message
data = bytes("Track1", "ascii")
ser.write(bytes([5]))
ser.write(data)
ser.write('\r'.encode())

while p.poll() is None:
    print("hello")

time.sleep(2.5)

p = Popen(['mpg123', '../test_audio/1.mp3'])
time.sleep(0.6)
# write the start message
data = bytes("Track2", "ascii")
ser.write(bytes([6]))
ser.write(data)
ser.write('\r'.encode())

while p.poll() is None:

    print("hello")

import serial 		#Install via https://pypi.org/project/pyserial/ 
import time
import sys

def Read(serialPort):
	while serialPort.in_waiting > 0:
		c = serialPort.read()
		if c == '\r':
			c = ' '
		sys.stdout.write(c)
	return

def Main():
	comPort = sys.argv[1]
	time.sleep(1)
	print ('Starting UART monitor @ ' + comPort)
	ser = serial.Serial(comPort, 115200, timeout=1)
	#Read endlesly
	while True:
		#time.sleep(1)
		Read(ser)

Main()
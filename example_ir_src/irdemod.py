import serial
import logging

#
# open serial port
#
ser = serial.Serial('com13',9600)
ser.setDTR()
   
ser.flushInput()
count = 0

# set up logging
LOG_FILENAME = 'C:/Documents and Settings/Inna/Desktop/IRLED/testIRdemod.txt'
logging.basicConfig(filename=LOG_FILENAME,level=logging.DEBUG,)

while 1:
   count += 1
   x = ser.read()

   # log value to file
   logging.debug(str(x))

   print '%d:' % count, x,   ' (dec %d' % ord(x),   ' hex %x)' % ord(x)

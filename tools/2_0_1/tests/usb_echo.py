# This is a rudimentary USB to STM32 test script that sends data and looks for the same data being returned
# The client must send any received data back

import logging
import time
import sys
from arribada_tools import pyusb

logging.basicConfig(stream=sys.stdout, level=logging.DEBUG) # Print logging output to stdout

try:
    host = pyusb.UsbHost()
except:
    print("Unexpected error: %s", sys.exc_info()[0])
    quit()

numOfAttempts = 100000
timeout = 10000000000

timeAtStart = time.time()
for i in range(0, numOfAttempts):
    stringToSend = str(i)
    host.write(pyusb.EP_MSG_OUT, stringToSend, timeout).wait() # Send test string
    event = host.read(pyusb.EP_MSG_IN, len(stringToSend), timeout) # Prepare to receive
    event.wait() # Receive data
    stringReceived = str(bytearray(event.buffer))
    if stringReceived != stringToSend: # Compare received to sent
        print("Strings are not equal!")
        print("String sent ", stringToSend)
        print("String received ", stringReceived)
        print("Test failed!")
        quit()

timeAtEnd = time.time()
timeElapsedInMs = int(round((timeAtEnd - timeAtStart) * 1000))
print("Test passed in", str(timeElapsedInMs), "ms")
del host
quit()
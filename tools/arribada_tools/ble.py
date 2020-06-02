from array import array
import binascii
import struct
import os
import logging
import bluepy.btle


config_service_uuid = bluepy.btle.UUID('04831523-6c9d-6ca9-5d41-03ad4fff4abb')
config_char_uuid = bluepy.btle.UUID('04831524-6c9d-6ca9-5d41-03ad4fff4abb')
MAX_PACKET_SIZE = 20
CONNECTION_TIMEOUT = 3

HCI_DEV = 0 if 'HCI_DEV' not in os.environ else int(os.environ['HCI_DEV'])

logger = logging.getLogger(__name__)


class Buffer():
    def __init__(self):
        self._buffer = b''

    def write(self, data):
        self._buffer = self._buffer + data

    def read(self, length):
        data = self._buffer[:length]
        self._buffer = self._buffer[length:]
        return data
    
    def occupancy(self):
        return len(self._buffer)


class MyDelegate(bluepy.btle.DefaultDelegate):

    def __init__(self, buf):
        bluepy.btle.DefaultDelegate.__init__(self)
        self._buffer = buf

    def handleNotification(self, cHandle, data):
        logger.debug("Received: %s", binascii.hexlify(array('B', data)))
        self._buffer.write(data)


def notifications_enable(p, char):
    for desc in p.getDescriptors(char.getHandle(), 0x00F): 
        if desc.uuid == 0x2902:
            ccc_handle = desc.handle
            p.writeCharacteristic(ccc_handle, struct.pack('<bb', 0x01, 0x00))
            break


class MyPeripheral(bluepy.btle.Peripheral):
    """
    Override default Peripheral class to add connection timeouts.  By
    default, this is not supported by the default class.
    """
    def __init__(self, deviceAddr=None, addrType=bluepy.btle.ADDR_TYPE_PUBLIC, iface=None, conn_timeout=None):
        self._conn_timeout = conn_timeout
        bluepy.btle.Peripheral.__init__(self, deviceAddr, addrType, iface)

    def _connect(self, addr, addrType=bluepy.btle.ADDR_TYPE_PUBLIC, iface=None):
        if len(addr.split(":")) != 6:
            raise ValueError("Expected MAC address, got %s" % repr(addr))
        if addrType not in (bluepy.btle.ADDR_TYPE_PUBLIC, bluepy.btle.ADDR_TYPE_RANDOM):
            raise ValueError("Expected address type public or random, got {}".format(addrType))
        self._startHelper()
        self.addr = addr
        self.addrType = addrType
        self.iface = iface
        if iface is not None:
            self._writeCmd("conn %s %s %s\n" % (addr, addrType, "hci"+str(iface)))
        else:
            self._writeCmd("conn %s %s\n" % (addr, addrType))
        rsp = self._getResp('stat', timeout=self._conn_timeout)
        while rsp['state'][0] == 'tryconn':
            rsp = self._getResp('stat', timeout=self._conn_timeout)
        if rsp['state'][0] != 'conn':
            self._stopHelper()
            raise bluepy.btle.BTLEException(bluepy.btle.BTLEException.DISCONNECTED,
                                "Failed to connect to peripheral %s, addr type: %s" % (addr, addrType))


class BluetoothTracker():
    def __init__(self, uuid, conn_timeout):
        self._buffer = Buffer()
        self._periph = MyPeripheral(uuid, 'random', HCI_DEV, conn_timeout=conn_timeout)
        self._periph.setDelegate(MyDelegate(self._buffer))
        self._config_service = self._periph.getServiceByUUID(config_service_uuid)
        self._config_char = self._config_service.getCharacteristics(config_char_uuid)[0]
        notifications_enable(self._periph, self._config_char)

    def write(self, data):
        while data: # Send data in discrete packets
            bytesToSend = min(len(data), MAX_PACKET_SIZE)
            self._config_char.write(data[:bytesToSend])
            logger.debug("Transmit: %s", binascii.hexlify(array('B', data[:bytesToSend])))
            data = data[bytesToSend:]

    def read(self, timeout=0):
        # Read just one packet
        if not self._buffer.occupancy():
            self._periph.waitForNotifications(timeout)
        return self._buffer.read(self._buffer.occupancy())

    def readFull(self, length, timeout=0):
        # Read up until length has been reached
        while self._buffer.occupancy() < length:
            self._periph.waitForNotifications(timeout)
        return self._buffer.read(length)

    def cleanup(self):
        self._periph.disconnect()

from . import pyusb
from . import message
import math
from array import array
try: # BLE is currently not supported on Windows
    from .ble import BluetoothTracker
except:
    pass
import time

class ExceptionBackendNotFound(Exception):
    pass


class ExceptionBackendNotImplemented(Exception):
    pass


class _Backend(object):

    def __init__(self, **kwargs):
        pass

    def command_response(self, command, timeout=None):
        pass

    def write(self, data, timeout=None):
        pass

    def read(self, length, timeout=None):
        pass

    def cleanup(self):
        pass


class BackendBluetooth(_Backend):

    def __init__(self, dev_addr=None, conn_timeout=None):
        try:
            self._ble = BluetoothTracker(dev_addr, conn_timeout=conn_timeout)
        except:
            raise ExceptionBackendNotFound

    def command_response(self, command, timeout=None):
        """Send a command (optionally) over USB and wait for a response to come back.
        The input command is a message object and any response shall be
        first decoded to a response message object.
        """

        CONST_INTER_PACKET_TIMEOUT = 0.5

        if command:
            resp = self._ble.write(command.pack())

        resp = None

        for _ in range(0, math.ceil(timeout / CONST_INTER_PACKET_TIMEOUT)):
            packet = self._ble.read(CONST_INTER_PACKET_TIMEOUT)
            if packet is not None:
                if resp is None:
                    resp = packet
                else:
                    resp = resp + packet
            elif resp is not None:
                break

        if resp == None:
            return resp
        
        else:
            (msg, _) = message.decode(array('B',resp))
            return msg

    def write(self, data, timeout=None):
        self._ble.write(data)
        return len(data)

    def read(self, length, timeout=None):
        return self._ble.readFull(length, timeout)

    def cleanup(self):
        self._ble.cleanup()


class BackendUsb(_Backend):

    def __init__(self, dev_index=0):
        try:
            self._usb = pyusb.UsbHost(dev_index)
            self._dev_index = dev_index
        except:
            raise ExceptionBackendNotFound

    def command_response(self, command, timeout=None):
        """Send a command (optionally) over USB and wait for a response to come back.
        The input command is a message object and any response shall be
        first decoded to a response message object.
        """

        if timeout != None:
            timeout = timeout * 1000 # Scale timeout to milliseconds
            timeout = int(timeout)

        if command:
            resp = self._usb.write(pyusb.EP_MSG_OUT, command.pack(), timeout)
            resp.wait()
            if resp.status == -1:
                return None
        # All command responses are guaranteed to fit inside 512 bytes
        resp = self._usb.read(pyusb.EP_MSG_IN, 512, timeout)
        resp.wait()
        if resp.status == -1:
            return None
        else:
            (msg, _) = message.decode(resp.buffer)
            return msg

    def write(self, data, timeout=None):
        """Write data transparently over USB in small chunks
        until all bytes are transmitted or a timeout occurs.  Returns
        the total number of bytes sent.
        """

        if timeout != None:
            timeout = timeout * 1000 # Scale timeout to milliseconds
            timeout = int(timeout)

        size = 0
        zero_byte_packet_sent = False
        while data:
            zero_byte_packet_sent = False
            resp = self._usb.write(pyusb.EP_MSG_OUT, data[:512], timeout)
            resp.wait()
            if resp.status <= 0:
                break
            data = data[resp.status:]
            size = size + resp.status
            if (resp.status == 512):
                zero_byte_packet_sent = True
                resp = self._usb.write(pyusb.EP_MSG_OUT, '', timeout) # See https://www.microchip.com/forums/m818567.aspx for why a zero length packet is required
                resp.wait()

        # Was the last packet we sent a multiple of 64, and have we also not already sent a zero packet size
        if (size % 64) == 0 and not zero_byte_packet_sent:
            resp = self._usb.write(pyusb.EP_MSG_OUT, '', timeout)
            resp.wait()

        return size

    def read(self, length, timeout=None):
        """Read data transparently over USB in small chunks
        until all bytes are received or a timeout occurs.  Returns
        the data buffer received.
        """

        if timeout != None:
            timeout = timeout * 1000 # Scale timeout to milliseconds
            timeout = int(timeout)

        data = array('B', [])
        while length > 0:
            resp = self._usb.read(pyusb.EP_MSG_IN, min(512, length), timeout)
            resp.wait()
            if resp.status == -1:
                break
            if len(resp.buffer):
                data = data + resp.buffer
            length = length - len(resp.buffer)
        return data

    def get_devices(self):
        return self._usb.get_devices()

    def get_dev_index(self):
        return self._dev_index

    def cleanup(self):
        self._usb.cleanup()

from . import message
from . import ubx
import logging
import serial
import time
import binascii
from array import *


logger = logging.getLogger(__name__)

_timeout = 1.0


class ExceptionGPSCommsTimeoutError(Exception):
    pass


class ExceptionGPSFlashError(Exception):
    pass


class ExceptionGPSConfigError(Exception):
    pass


class GPSSerialBackend(object):
    """Use a serial backend"""
    def __init__(self, path, baudrate, timeout=0):
        self._serial = serial.Serial(port=path,
                                     baudrate=baudrate,
                                     bytesize=8,
                                     parity=serial.PARITY_NONE,
                                     stopbits=serial.STOPBITS_ONE,
                                     timeout=timeout)

    def read(self, length=512):
        return self._serial.read(length)

    def write(self, data):
        self._serial.write(data)

    def cleanup(self):
        pass


class GPSBridgedBackend(object):
    """Use a USB/BLE bridged backend"""

    def __init__(self, backend):
        self._backend = backend

    def read(self, length=512):
        #time.sleep(0.15)
        cmd = message.ConfigMessage_GPS_READ_REQ(length=length)
        resp = self._backend.command_response(cmd, _timeout)
        if not resp or resp.name != 'GPS_READ_RESP' or resp.error_code:
            logger.error('Bad response to GPS_READ_REQ')
            raise ExceptionGPSCommsTimeoutError
        if (resp.length > 0):
            return self._backend.read(resp.length, _timeout)
        return b''

    def write(self, data):
        #time.sleep(0.15)
        cmd = message.ConfigMessage_GPS_WRITE_REQ(length=len(data));
        resp = self._backend.command_response(cmd, _timeout)
        if not resp or resp.name != 'GENERIC_RESP' or resp.error_code:
            logger.error('Bad response to GPS_WRITE_REQ')
            raise ExceptionGPSCommsTimeoutError
        self._backend.write(data, _timeout)

    def cleanup(self):
        self._backend.cleanup


class GPSConfig(object):
    """GPS configuration wrapper class for u-blox M8N"""
    def __init__(self, gps_backend):
        # See GPSBridgedBackend and GPSSerialBackend
        self._backend = gps_backend

    def _wait_for_ack(self):
        data = array('B', [])
        retries = 300
        while retries > 0:
            data = array('B', data) + array('B', self._backend.read())
            while True:
                (msg, data) = ubx.ubx_extract(data)
                if not msg:
                    time.sleep(0.01)
                    break
                idstr = ubx.ubx_to_string(msg)
                logger.debug('RX: %s', idstr)
                if idstr == 'ACK-ACK':
                    return True
                elif idstr == 'ACK-NAK':
                    return False
            retries = retries - 1
        raise ExceptionGPSCommsTimeoutError

    def _wait_for_mga_flash_ack(self, expected_sequence):
        data = array('B', [])
        retries = 500
        while retries > 0:
            data = array('B', data) + array('B', self._backend.read())
            while True:
                (msg, data) = ubx.ubx_extract(data)
                if not msg:
                    time.sleep(0.01)
                    break
                idstr = ubx.ubx_to_string(msg)
                logger.debug('RX: %s', idstr)
                if idstr == 'MGA-FLASH':
                    (ack, sequence) = ubx.ubx_mga_flash_ack_extract(msg)
                    if expected_sequence != sequence or ack == 2:
                        logger.error('RX: MGA-FLASH failure: expected=%u actual=%u ack=%u',
                                     expected_sequence, sequence, ack)
                        raise ExceptionGPSFlashError
                    logger.debug('RX: MGA-FLASH: seq=%u ack=%u', sequence, ack)
                    return ack == 0
            retries = retries - 1
        raise ExceptionGPSCommsTimeoutError

    def mga_ano_session(self, mga_ano_data):
        """MGA AssistNowOffline session"""
        #self._backend.write(ubx.ubx_mga_flash_stop())
        #self._wait_for_mga_flash_ack(0xFFFF)
        sequence = 0
        while True:
            mga_ano = b''
            for _ in range(5):
                (m, mga_ano_data) = ubx.ubx_extract(mga_ano_data)
                if not m:
                    break
                mga_ano = mga_ano + m
            if mga_ano:
                msg = ubx.ubx_mga_flash_data(sequence, mga_ano)
                retries = 3
                while retries > 0:
                    logger.debug('TX: %s: len=%u: seq=%u', ubx.ubx_to_string(msg), len(msg), sequence)
                    self._backend.write(msg)
                    if self._wait_for_mga_flash_ack(sequence):
                        break
                    retries = retries - 1
                if retries == 0:
                    logger.error('MGA-FLASH failed at sequence=%u', sequence)
                    raise ExceptionGPSFlashError
            else:
                break
            sequence = sequence + 1
            logger.debug('%u bytes remaining', len(mga_ano_data))
        self._backend.write(ubx.ubx_mga_flash_stop())
        self._wait_for_mga_flash_ack(0xFFFF)

    def ascii_config_session(self, text):
        """ASCII text configuration session"""
        for line in text.splitlines():
            msg = ubx.ubx_build_from_ascii_cfg(line)
            if msg:
                logger.debug('TX: %s: len=%u: %s', ubx.ubx_to_string(msg), len(msg), binascii.hexlify(msg))
                self._backend.write(msg)
                if not self._wait_for_ack():
                    logger.warn('NAK: %s', line)
                else:
                    logger.debug('ACK: %s', line)
            else:
                logger.warn('Not CFG: %s', line)

        # Save configuration to flash
        msg = ubx.ubx_cfg_save_flash()
        logger.debug('TX: %s: len=%u: %s', ubx.ubx_to_string(msg), len(msg), binascii.hexlify(msg))
        self._backend.write(msg)
        if not self._wait_for_ack():
            raise ExceptionGPSFlashError

import hashlib
from . import message
import logging
import serial
import time

logger = logging.getLogger(__name__)


# Default timeout on all commands
_timeout = 2.0


class ExceptionCellularCommsError(Exception):
    pass


class ExceptionCellularUnexpectedResponse(Exception):
    pass


class CellularSerialBackend(object):
    """Use a serial backend"""
    def __init__(self, path, baudrate, timeout=_timeout):
        self._serial = serial.Serial(port=path,
                                     baudrate=baudrate,
                                     bytesize=8,
                                     parity=serial.PARITY_NONE,
                                     stopbits=serial.STOPBITS_ONE,
                                     timeout=timeout,
                                     rtscts=True,
                                     inter_byte_timeout=None)

    def read_until(self, length=512, expected='\r\n', timeout=_timeout):
        self._serial.timeout = timeout
        return self._serial.read_until(expected, length)

    def read(self, length=512, timeout=_timeout):
        self._serial.timeout = timeout
        return self._serial.read(length)

    def write(self, data):
        self._serial.write(data)

    def flush(self):
        self._serial.reset_input_buffer()
        
    def cleanup(self):
        pass


class CellularBridgedBackend(object):
    """Use a USB/BLE bridged backend"""

    def __init__(self, backend):
        self._backend = backend

    def flush(self):
        while (self.read(timeout=1.0)):
            pass
        
    def read_until(self, length=512, expected='\r\n', timeout=_timeout):
        data = b''
        t_start = time.time()
        while True:
            cmd = message.ConfigMessage_CELLULAR_READ_REQ(length=length)
            resp = self._backend.command_response(cmd, timeout)
            if not resp or resp.name != 'CELLULAR_READ_RESP' or resp.error_code:
                logger.error('Bad response to CELLULAR_READ_REQ')
                raise ExceptionCellularCommsError
            if (resp.length > 0):
                new_data = self._backend.read(resp.length, timeout)
                new_data = ''.join(map(chr, new_data))
                data = data + new_data
                if expected in data:
                    return data
                t_start = time.time()
            elif (time.time() - t_start) >= timeout:
                return b''

    def read(self, length=512, timeout=_timeout):
        cmd = message.ConfigMessage_CELLULAR_READ_REQ(length=length)
        resp = self._backend.command_response(cmd, timeout)
        if not resp or resp.name != 'CELLULAR_READ_RESP' or resp.error_code:
            logger.error('Bad response to CELLULAR_READ_REQ')
            raise ExceptionCellularCommsError()
        if (resp.length > 0):
            return self._backend.read(resp.length, timeout)
        return b''

    def write(self, data, timeout=_timeout):
        cmd = message.ConfigMessage_CELLULAR_WRITE_REQ(length=len(data));
        resp = self._backend.command_response(cmd, timeout)
        if not resp or resp.name != 'GENERIC_RESP' or resp.error_code:
            logger.error('Bad response to CELLULAR_WRITE_REQ')
            raise ExceptionCellularCommsError()
        self._backend.write(data, timeout)

    def cleanup(self):
        self._backend.cleanup


class CellularConfig(object):
    """Cellular configuration wrapper class for AT commands to SARA-U270"""
    def __init__(self, cellular_backend):
        # See CellularBridgedBackend and CellularSerialBackend
        self._backend = cellular_backend
        self._sync_comms()
        self._disable_local_echo()

    def _expect(self, expected, timeout=_timeout):
        resp = self._backend.read_until(expected=expected, timeout=timeout)
        logger.debug('read: %s exp: %s', resp.strip(), expected)
        if resp:
            if expected not in resp:
                raise ExceptionCellularUnexpectedResponse('Got %s but expected %s' % (resp, expected))
        else:
            raise ExceptionCellularCommsError()

    def _flush(self):
        self._backend.flush()

    def _disable_local_echo(self):
        self._flush()
        self._backend.write('ATE0\r')
        self._expect('OK')

    def _sync_comms(self):
        retries = 3
        while retries:
            self._flush()
            self._backend.write('AT\r')
            try:
                self._expect('OK')
            except Exception as e:
                if retries == 0:
                    raise e
                retries = retries - 1
            else:
                break

    def _delete(self, index, name):
        cmd = 'AT+USECMNG=2,%u,"%s"\r' % (index, name)
        logger.debug('send: %s', cmd.strip())
        self._flush()
        self._backend.write(cmd)
        self._expect('OK')
        logger.info('%s removed successfully', name)

    def _create(self, index, name, data):
        cmd = 'AT+USECMNG=0,%u,"%s",%u\r' % (index, name, len(data))
        logger.debug('send: %s', cmd.strip())
        self._flush()
        self._backend.write(cmd)
        self._expect('>')
        self._backend.write(data)
        md5sum = hashlib.md5(data).hexdigest()
        self._expect('+USECMNG: 0,%u,"%s","%s"\r\n\r\nOK' % (index, name, md5sum))
        logger.info('%s added successfully', name)

    def _verify(self, index, name, data):
        cmd = 'AT+USECMNG=4,%u,"%s"\r' % (index, name)
        logger.debug('send: %s', cmd.strip())
        self._flush()
        self._backend.write(cmd)
        md5sum = hashlib.md5(data).hexdigest()
        self._expect('+USECMNG: 4,%u,"%s","%s"\r\n\r\nOK' % (index, name, md5sum))
        logger.info('%s verified successfully', name)

    def delete_all(self):
        try:
            self._delete(0, 'root-CA.pem')
        except:
            logger.warn('Unable to remove root-CA.pem')
        try:
            self._delete(1, 'deviceCert.pem')
        except:
            logger.warn('Unable to remove deviceCert.pem')
        try:
            self._delete(2, 'deviceCert.key')
        except:
            logger.warn('Unable to remove deviceCert.key')

    def create_all(self, root_ca, cert, key):
        self._create(0, 'root-CA.pem', root_ca)
        self._create(1, 'deviceCert.pem', cert)
        self._create(2, 'deviceCert.key', key)

    def verify_all(self, root_ca, cert, key):
        self._verify(0, 'root-CA.pem', root_ca)
        self._verify(1, 'deviceCert.pem', cert)
        self._verify(2, 'deviceCert.key', key)

    def disable_auto_attach(self):
        cmd = 'AT+COPS=2\r'
        logger.debug('send: %s', cmd.strip())
        self._backend.write(cmd)
        # COPS=2 may take up to 30 seconds
        self._expect('OK', timeout=30)
        logger.info('COPS=2 set successfully')
        cmd = 'AT&W0\r'
        logger.debug('send: %s', cmd.strip())
        self._backend.write(cmd)
        self._expect('OK')
        logger.info('Settings saved successfully')

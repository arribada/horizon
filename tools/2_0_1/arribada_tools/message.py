import struct
import logging
import inspect
import sys
import binascii


logger = logging.getLogger(__name__)


class ExceptionMessageInvalidValue(Exception):
    pass


def str_error(error_code):
    errors = [ 'CMD_NO_ERROR',
      'CMD_ERROR_FILE_NOT_FOUND',
      'CMD_ERROR_FILE_ALREADY_EXISTS',
      'CMD_ERROR_INVALID_CONFIG_TAG',
      'CMD_ERROR_GPS_COMMS',
      'CMD_ERROR_TIMEOUT',
      'CMD_ERROR_CONFIG_PROTECTED',
      'CMD_ERROR_CONFIG_TAG_NOT_SET',
      'CMD_ERROR_BRIDGING_DISABLED',
      'CMD_ERROR_DATA_OVERSIZE',
      'CMD_ERROR_INVALID_PARAMETER',
      'CMD_ERROR_INVALID_FW_IMAGE_TYPE',
      'CMD_ERROR_IMAGE_CRC_MISMATCH',
      'CMD_ERROR_FILE_INCOMPATIBLE',
      'CMD_ERROR_CELLULAR_COMMS'
      ]
    if error_code >= len(errors):
        return 'ERROR_UNKNOWN'
    return errors[error_code]


def decode(data):

    """Attempt to decode a message from an input data buffer"""
    hdr = ConfigMessageHeader()
    if (len(data) < hdr.header_length):
        return (None, data)

    # If there's no sync byte return
    if ConfigMessageHeader.sync not in data:
        return (None, data)

    # Find location of first sync byte and decode from that position
    pos = data.index(ConfigMessageHeader.sync)

    # Unpack message header at SYNC position
    hdr.unpack(data[pos:])

    msg = None
    unused_cls = [ ConfigMessage, ConfigMessageHeader ]
    for i in inspect.getmembers(sys.modules[__name__], inspect.isclass):
        cls = i[1]
        if issubclass(cls, ConfigMessageHeader) and cls not in unused_cls and \
            hdr.cmd == cls.cmd:
            msg = cls()
            break

    if (msg):
        msg.unpack(data)
        # Advance buffer past this message
        data = data[(pos + msg.length):]

    return (msg, data)


def convert_to_dict(obj):
    d = {}
    ignore = ['error_code', 'cmd', 'sync']
    for i in obj._args:
        if i not in ignore:
            d[i] = getattr(obj, i)
    return d


def version_int_to_str(version):
    version = int(version)

    if version == 0xFFFFFFFF:
        return "unknown"

    major = (version >> 24) & 0xFF
    minor = (version >> 16) & 0xFF
    tweak = (version >> 8) & 0xFF
    patch = version & 0xFF

    if tweak == 0:
        tweak = "a" # Alpha
    elif tweak == 1:
        tweak = "b" # Beta
    elif tweak == 2:
        tweak = "rc" # Release candidate
    elif tweak == 3:
        tweak = "r" # Release
    else:
        tweak = "?"

    if patch != 0:
        return str(major) + '.' + str(minor) + tweak + str(patch)
    else:
        return str(major) + '.' + str(minor) + tweak


class _Blob(object):
    """Blob object is a container for arbitrary message fields which
    can be packed / unpacked using python struct"""
    _fmt = ''
    _args = []

    def __init__(self, fmt, args):
        self._fmt = fmt
        self._args = args

    def extend(self, fmt, args):
        self._fmt += fmt
        self._args += args

    def pack(self):
        packer = struct.Struct(self._fmt)
        args = tuple([getattr(self, k) for k in self._args])
        return packer.pack(*args)

    def unpack(self, data):
        unpacker = struct.Struct(self._fmt)
        unpacked = unpacker.unpack_from(data)
        i = 0
        for k in self._args:
            setattr(self, k, unpacked[i])
            i += 1

    def __repr__(self):
        s = self.__class__.__name__ + ' contents:\n'
        for i in self._args:
            s += i + ' = ' + str(getattr(self, i, 'undefined')) + '\n'
        return s


class ConfigMessageHeader(_Blob):
    """Configuration message header"""
    sync = 0x7E

    def __init__(self, bytes_to_follow=0):
        _Blob.__init__(self, b'<BB', ['sync', 'cmd'])
        self.header_length = struct.calcsize(self._fmt)
        self.length = self.header_length + bytes_to_follow


class ConfigMessage(ConfigMessageHeader):
    """A configuration message which should be subclassed"""
    def __init__(self, fmt=b'', args=[], **kwargs):
        ConfigMessageHeader.__init__(self, struct.calcsize(fmt))
        self.extend(fmt, args)
        for k in list(kwargs.keys()):
            setattr(self, k, kwargs[k])


class GenericResponse(ConfigMessage):
    """A generic response message which should be subclassed"""
    cmd = 0
    name = 'GENERIC_RESP'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'B', ['error_code'], **kwargs)


class ConfigMessage_CFG_READ_REQ(ConfigMessage):

    cmd = 1
    name = 'CFG_READ_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'H', ['cfg_tag'], **kwargs)


class ConfigMessage_CFG_READ_RESP(ConfigMessage):

    cmd = 8
    name = 'CFG_READ_RESP'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'BI', ['error_code', 'length'], **kwargs)


class ConfigMessage_CFG_WRITE_CNF(ConfigMessage):

    cmd = 9
    name = 'CFG_WRITE_CNF'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'B', ['error_code'], **kwargs)


class ConfigMessage_CFG_WRITE_REQ(ConfigMessage):

    cmd = 2
    name = 'CFG_WRITE_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'I', ['length'], **kwargs)


class ConfigMessage_CFG_SAVE_REQ(ConfigMessageHeader):

    cmd = 3
    name = 'CFG_SAVE_REQ'


class ConfigMessage_CFG_RESTORE_REQ(ConfigMessageHeader):

    cmd = 4
    name = 'CFG_RESTORE_REQ'


class ConfigMessage_CFG_ERASE_REQ(ConfigMessage):

    cmd = 5
    name = 'CFG_ERASE_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'H', ['cfg_tag'], **kwargs)


class ConfigMessage_CFG_PROTECT_REQ(ConfigMessageHeader):

    cmd = 6
    name = 'CFG_PROTECT_REQ'


class ConfigMessage_CFG_UNPROTECT_REQ(ConfigMessageHeader):

    cmd = 7
    name = 'CFG_UNPROTECT_REQ'


class ConfigMessage_GPS_WRITE_REQ(ConfigMessage):

    cmd = 10
    name = 'GPS_WRITE_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'I', ['length'], **kwargs)


class ConfigMessage_GPS_READ_REQ(ConfigMessage):

    cmd = 11
    name = 'GPS_READ_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'I', ['length'], **kwargs)


class ConfigMessage_GPS_READ_RESP(ConfigMessage):

    cmd = 12
    name = 'GPS_READ_RESP'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'BI', ['error_code', 'length'], **kwargs)


class ConfigMessage_GPS_CONFIG_REQ(ConfigMessage):

    cmd = 13
    name = 'GPS_CONFIG_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'?', ['enable'], **kwargs)


class ConfigMessage_BLE_CONFIG_REQ(ConfigMessage):

    cmd = 14
    name = 'BLE_CONFIG_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'?', ['enable'], **kwargs)


class ConfigMessage_BLE_WRITE_REQ(ConfigMessage):

    cmd = 15
    name = 'BLE_WRITE_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'BH', ['address', 'length'], **kwargs)


class ConfigMessage_BLE_READ_REQ(ConfigMessage):

    cmd = 16
    name = 'BLE_READ_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'BH', ['address', 'length'], **kwargs)


class ConfigMessage_STATUS_REQ(ConfigMessageHeader):

    cmd = 17
    name = 'STATUS_REQ'


class ConfigMessage_STATUS_RESP(ConfigMessage):

    cmd = 18
    name = 'STATUS_RESP'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'B???15sx?',
                               ['error_code',
                                'gps_module_detected',
                                'cellular_module_detected',
                                'sim_card_present',
                                'sim_card_imsi',
                                'satellite_module_detected'
                                ], **kwargs)

    def unpack(self, data):
        ConfigMessage.unpack(self, data)
        if not self.cellular_module_detected:
            self.sim_card_imsi = "N/A"
            self.sim_card_present = "N/A"


class ConfigMessage_FW_SEND_IMAGE_REQ(ConfigMessage):

    cmd = 19
    name = 'FW_SEND_IMAGE_REQ'

    # First two entries are reserved for future use
    allowed_image_type = ['', '', 'ARTIC']
    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'BII', ['image_type', 'image_length', 'crc',], **kwargs)

    def pack(self):
        if hasattr(self, 'image_type'):
            image_type = self.image_type
            if image_type and image_type in self.allowed_image_type:
                self.image_type = self.allowed_image_type.index(image_type)
            else:
                raise ExceptionMessageInvalidValue('image_type must be one of %s' %
                                                   [i for i in self.allowed_image_type if i])
        else:
            raise ExceptionMessageInvalidValue('image_type is a mandatory parameter')
        if not hasattr(self, 'image_length'):
            raise ExceptionMessageInvalidValue('image_length is a mandatory parameter')
        if not hasattr(self, 'crc'):
            raise ExceptionMessageInvalidValue('crc is a mandatory parameter')

        data = ConfigMessage.pack(self)
        self.image_type = image_type
        return data

    def unpack(self, data):
        ConfigMessage.unpack(self, data)
        try:
            self.image_type = self.allowed_image_type[self.image_type]
        except:
            self.image_type = 'UNKNOWN'


class ConfigMessage_FW_SEND_IMAGE_COMPLETE_CNF(GenericResponse):

    cmd = 20
    name = 'FW_SEND_IMAGE_COMPLETE_CNF'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'B', ['error_code'], **kwargs)


class ConfigMessage_FW_APPLY_IMAGE_REQ(ConfigMessage):

    cmd = 21
    name = 'FW_APPLY_IMAGE_REQ'
    allowed_image_type = ['', '', 'ARTIC']

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'B', ['image_type'], **kwargs)

    def pack(self):
        if hasattr(self, 'image_type'):
            image_type = self.image_type
            if image_type and image_type in self.allowed_image_type:
                self.image_type = self.allowed_image_type.index(image_type)
            else:
                raise ExceptionMessageInvalidValue('image_type must be one of %s' %
                                                   [i for i in self.allowed_image_type if i])
        else:
            raise ExceptionMessageInvalidValue('image_type is a mandatory parameter')
        data = ConfigMessage.pack(self)
        self.image_type = image_type
        return data

    def unpack(self, data):
        ConfigMessage.unpack(self, data)
        try:
            self.image_type = self.allowed_image_type[self.image_type]
        except:
            self.image_type = 'UNKNOWN'


class ConfigMessage_RESET_REQ(ConfigMessage):

    cmd = 22
    name = 'RESET_REQ'
    allowed_reset = ['CPU', 'FLASH', 'DFU']

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'B', ['reset_type'], **kwargs)

    def pack(self):
        if hasattr(self, 'reset_type'):
            reset_type = self.reset_type
            if reset_type in self.allowed_reset:
                self.reset_type = self.allowed_reset.index(reset_type)
            else:
                raise ExceptionMessageInvalidValue('reset_type must be one of %s' % self.allowed_reset)
        else:
            raise ExceptionMessageInvalidValue('reset_type is a mandatory parameter')

        data = ConfigMessage.pack(self)
        self.reset_type = reset_type
        return data

    def unpack(self, data):
        ConfigMessage.unpack(self, data)
        try:
            self.reset_type = self.allowed_reset[self.reset_type]
        except:
            self.reset_type = 'UNKNOWN'


class ConfigMessage_BATTERY_STATUS_REQ(ConfigMessageHeader):

    cmd = 23
    name = 'BATTERY_STATUS_REQ'


class ConfigMessage_BATTERY_STATUS_RESP(ConfigMessage):

    cmd = 24
    name = 'BATTERY_STATUS_RESP'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'B?BH', ['error_code', 'charging_ind', 'charging_level', 'millivolts'], **kwargs)


class ConfigMessage_LOG_CREATE_REQ(ConfigMessage):

    cmd = 25
    name = 'LOG_CREATE_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'B?', ['mode', 'sync_enable'], **kwargs)

    def pack(self):
        mode = self.mode
        if self.mode == 'LINEAR':
            self.mode = 0
        elif self.mode == 'CIRCULAR':
            self.mode = 1
        else:
            raise ExceptionMessageInvalidValue
        data = ConfigMessage.pack(self)
        self.mode = mode
        return data

    def unpack(self, data):
        ConfigMessage.unpack(self, data)
        if (self.mode == 0):
            self.mode = 'LINEAR'
        elif (self.mode == 1):
            self.mode = 'CIRCULAR'
        else:
            self.mode = 'UNKNOWN'


class ConfigMessage_LOG_ERASE_REQ(ConfigMessageHeader):

    cmd = 26
    name = 'LOG_ERASE_REQ'


class ConfigMessage_LOG_READ_REQ(ConfigMessage):

    cmd = 27
    name = 'LOG_READ_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'II', ['start_offset', 'length'], **kwargs)


class ConfigMessage_LOG_READ_RESP(ConfigMessage):

    cmd = 28
    name = 'LOG_READ_RESP'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'BI', ['error_code', 'length'], **kwargs)


class ConfigMessage_CELLULAR_CONFIG_REQ(ConfigMessage):

    cmd = 29
    name = 'CELLULAR_CONFIG_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'?', ['enable'], **kwargs)


class ConfigMessage_CELLULAR_WRITE_REQ(ConfigMessage):

    cmd = 30
    name = 'CELLULAR_WRITE_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'I', ['length'], **kwargs)


class ConfigMessage_CELLULAR_READ_REQ(ConfigMessage):

    cmd = 31
    name = 'CELLULAR_READ_REQ'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'I', ['length'], **kwargs)


class ConfigMessage_CELLULAR_READ_RESP(ConfigMessage):

    cmd = 32
    name = 'CELLULAR_READ_RESP'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'BI', ['error_code', 'length'], **kwargs)


class ConfigMessage_TEST_REQ(ConfigMessage):

    cmd = 33
    name = 'TEST_REQ'
    allowed_test_mode = ['GPS', 'CELLULAR', 'SATELLITE']

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'B', ['test_mode'], **kwargs)

    def pack(self):
        if hasattr(self, 'test_mode'):
            old_test_mode = self.test_mode
            test_mode = 0
            for i in self.test_mode:
                if i in self.allowed_test_mode:
                    test_mode = test_mode | (1<<self.allowed_test_mode.index(i))
                else:
                    raise ExceptionMessageInvalidValue('test_mode must be one of %s', self.allowed_test_mode)                
            self.test_mode = test_mode
        else:
            raise ExceptionMessageInvalidValue('test_mode is a mandatory parameter')

        data = ConfigMessage.pack(self)
        self.test_mode = old_test_mode
        return data

    def unpack(self, data):
        ConfigMessage.unpack(self, data)
        test_mode = []
        for i in self.allowed_test_mode:
            if self.test_mode & (1<<self.allowed_test_mode.index(i)):
                test_mode.append(i)
        self.test_mode = test_mode


class ConfigMessage_FLASH_DOWNLOAD_REQ(ConfigMessageHeader):

    cmd = 34
    name = 'FLASH_DOWNLOAD_REQ'


class ConfigMessage_FLASH_DOWNLOAD_RESP(ConfigMessage):

    cmd = 35
    name = 'FLASH_DOWNLOAD_RESP'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'BI', ['error_code', 'length'], **kwargs)

class ConfigMessage_VERSION_REQ(ConfigMessageHeader):

    cmd = 36
    name = 'VERSION_REQ'


class ConfigMessage_VERSION_RESP(ConfigMessage):

    cmd = 37
    name = 'VERSION_RESP'

    def __init__(self, **kwargs):
        ConfigMessage.__init__(self, b'BH15sxIIIIQ',
                               ['error_code',
                                'hw_version',
                                'app_git_hash',
                                'app_version',
                                'ble_version',
                                'bootloader_version',
                                'cfg_version',
                                'unique_device_identifier'
                                ], **kwargs)

    def pack(self):
        old_id = self.unique_device_identifier[:]
        if len(old_id) > 16:
            raise ExceptionMessageInvalidValue('unique_device_identifier should not exceed 16 bytes in length')
        self.unique_device_identifier = int(old_id, 16)
        data = ConfigMessage.pack(self)
        self.unique_device_identifier = old_id
        return data

    def unpack(self, data):
        ConfigMessage.unpack(self, data)

        self.unique_device_identifier = binascii.hexlify(struct.pack('>Q', self.unique_device_identifier))

        self.app_version = version_int_to_str(self.app_version)
        self.bootloader_version = version_int_to_str(self.bootloader_version)

        self.app_git_hash = self.app_git_hash.decode("utf-8")
        self.unique_device_identifier = self.unique_device_identifier.decode("utf-8")

        hw_version_i = int(self.hw_version)
        self.hw_version = str((hw_version_i >> 8) & 0xFF) + "." + str(hw_version_i & 0xFF)

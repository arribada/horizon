import struct
import logging
import sys
import inspect


logger = logging.getLogger(__name__)


class ExceptionLogInvalidValue(Exception):
    pass


def decode(data, offset):
    """Attempt to decode a single log item from an input data buffer.
    A tuple is returned containing an instance of the log object plus
    the input data buffer, less the amount of data consumed."""
    item = TaggedItem()

    if ((len(data) + offset) < item.header_length):
        return None

    # Unpack header tag at current position
    item.unpack(data[offset:offset + item.header_length])
    # Find the correct configuration class based on the configuration tag
    cfg = None
    for i in inspect.getmembers(sys.modules[__name__], inspect.isclass):
        cls = i[1]
        if issubclass(cls, LogItem) and cls != LogItem and \
            item.tag == cls.tag:
            cfg = cls()
            break
    if (cfg):
        try:
            cfg.unpack(data[offset:offset + cfg.length])
        except:
            return None # Likely insufficient bytes to unpack

    return cfg


def decode_all(data):
    """Iteratively decode an input data buffer to a list of log
    objects.
    """
    objects = []
    offset = 0
    while offset < len(data):
        cfg = decode(data, offset)
        if cfg:
            objects += [ cfg ]
            offset += cfg.length
        else:
            break
    if offset != len(data):
        sys.stderr.write('Log file corruption detected at byte ' + str(offset) + '\n')
    return objects


def encode_all(objects):
    """Encode a list of log objects, in order, to a serial byte
    stream.
    """
    data = b''
    for i in objects:
        data += i.pack()
    return data


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


class TaggedItem(_Blob):
    """Tagged item bare (without any value)"""
    def __init__(self, bytes_to_follow=0):
        _Blob.__init__(self, b'<B', ['tag'])
        self.header_length = struct.calcsize(self._fmt)
        self.length = self.header_length + bytes_to_follow


class LogItem(TaggedItem):
    """A log item which should be subclassed"""
    def __init__(self, fmt=b'', args=[], **kwargs):
        TaggedItem.__init__(self, struct.calcsize(b'<' + fmt))
        self.extend(fmt, args)
        for k in list(kwargs.keys()):
            setattr(self, k, kwargs[k])


class LogItem_Builtin_LogStart(LogItem):
    tag = 0x7E
    name = 'LogStart'


class LogItem_Builtin_LogEnd(LogItem):
    tag = 0x7F
    name = 'LogEnd'
    fields = ['parity']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'B', self.fields, **kwargs)


class LogItem_GPS_Position(LogItem):
    tag = 0x00
    name = 'GPSPosition'
    fields = ['iTOW', 'longitude', 'latitude', 'height', 'accuracyHorizontal', 'accuracyVertical']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'IlllII', self.fields, **kwargs)

    def pack(self):
        longitude = self.longitude
        latitude = self.latitude
        height = self.height
        accuracyHorizontal = self.accuracyHorizontal
        accuracyVertical = self.accuracyVertical
        self.longitude = int(self.longitude / 1E-7)
        self.latitude = int(self.latitude / 1E-7)
        self.height = int(self.height * 1000.0)
        self.accuracyHorizontal = int(self.accuracyHorizontal * 1000.0)
        self.accuracyVertical = int(self.accuracyVertical * 1000.0)
        data = LogItem.pack(self)
        self.longitude = longitude
        self.latitude = latitude
        self.height = height
        self.accuracyHorizontal = accuracyHorizontal
        self.accuracyVertical = accuracyVertical
        return data

    def unpack(self, data):
        LogItem.unpack(self, data)
        self.longitude = 1E-7 * self.longitude
        self.latitude = 1E-7 * self.latitude
        self.height = self.height / 1000.0
        self.accuracyHorizontal = self.accuracyHorizontal / 1000.0
        self.accuracyVertical = self.accuracyVertical / 1000.0


class LogItem_GPS_TimeToFirstFix(LogItem):
    tag = 0x01
    name = 'TimeToFirstFix'
    fields = ['ttff']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'I', self.fields, **kwargs)


class LogItem_Pressure_Pressure(LogItem):
    tag = 0x02
    name = 'Pressure'
    fields = ['pressure']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'i', self.fields, **kwargs)

    def pack(self):
        pressure = self.pressure
        self.pressure = int(self.pressure * 1000.0)
        data = LogItem.pack(self)
        self.pressure = pressure
        return data

    def unpack(self, data):
        LogItem.unpack(self, data)
        self.pressure = self.pressure / 1000.0

class LogItem_AXL_XYZ(LogItem):
    tag = 0x03
    name = 'Accelerometer'
    fields = ['x', 'y', 'z']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'3h', self.fields, **kwargs)


class LogItem_Time_Timestamp(LogItem):
    tag = 0x12
    name = 'Timestamp'
    fields = ['timestamp']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'I', self.fields, **kwargs)


class LogItem_Time_DateTime(LogItem):
    tag = 0x04
    name = 'DateTime'
    fields = ['year', 'month', 'day', 'hours', 'minutes', 'seconds']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'H5B', self.fields, **kwargs)


class LogItem_Time_HighResTimer(LogItem):
    tag = 0x05
    name = 'HighResTimer'
    fields = ['hrt']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'Q', self.fields, **kwargs)


class LogItem_Temperature_Temperature(LogItem):
    tag = 0x06
    name = 'Temperature'
    fields = ['temperature']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'H', self.fields, **kwargs)


class LogItem_SaltwaterSwitch_Surfaced(LogItem):
    tag = 0x07
    name = 'Surfaced'


class LogItem_SaltwaterSwitch_Submerged(LogItem):
    tag = 0x08
    name = 'Submerged'


class LogItem_Battery_Charge(LogItem):
    tag = 0x09
    name = 'BatteryCharge'
    fields = ['charge']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'B', self.fields, **kwargs)


class LogItem_Battery_Voltage(LogItem):
    tag = 0x13
    name = 'BatteryVoltage'
    fields = ['voltage']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'H', self.fields, **kwargs)


class LogItem_Bluetooth_Enabled(LogItem):
    tag = 0x0A
    name = 'BluetoothEnabled'
    fields = ['cause']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'B', self.fields, **kwargs)

    def pack(self):
        cause = self.cause
        if self.cause == 'REED_SWITCH':
            self.cause = 0
        elif self.cause == 'SCHEDULE_TIMER':
            self.cause = 1
        elif self.cause == 'GEOFENCE':
            self.cause = 2
        else:
            raise ExceptionLogInvalidValue
        data = LogItem.pack(self)
        self.cause = cause
        return data

    def unpack(self, data):
        LogItem.unpack(self, data)
        if (self.cause == 0):
            self.cause = 'REED_SWITCH'
        elif (self.cause == 1):
            self.cause = 'SCHEDULE_TIMER'
        elif (self.cause == 2):
            self.cause = 'GEOFENCE'
        else:
            self.cause = 'UNKNOWN'


class LogItem_Bluetooth_Disabled(LogItem):
    tag = 0x0B
    name = 'BluetoothDisabled'
    fields = ['cause']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'B', self.fields, **kwargs)

    def pack(self):
        cause = self.cause
        if self.cause == 'REED_SWITCH':
            self.cause = 0
        elif self.cause == 'SCHEDULE_TIMER':
            self.cause = 1
        elif self.cause == 'GEOFENCE':
            self.cause = 2
        elif self.cause == 'INACTIVITY_TIMEOUT':
            self.cause = 3
        else:
            raise ExceptionLogInvalidValue
        data = LogItem.pack(self)
        self.cause = cause
        return data

    def unpack(self, data):
        LogItem.unpack(self, data)
        if (self.cause == 0):
            self.cause = 'REED_SWITCH'
        elif (self.cause == 1):
            self.cause = 'SCHEDULE_TIMER'
        elif (self.cause == 2):
            self.cause = 'GEOFENCE'
        elif (self.cause == 3):
            self.cause = 'INACTIVITY_TIMEOUT'
        else:
            self.cause = 'UNKNOWN'


class LogItem_Bluetooth_Connected(LogItem):
    tag = 0x0C
    name = 'BluetoothConnected'


class LogItem_Bluetooth_Disconnected(LogItem):
    tag = 0x0D
    name = 'BluetoothDisconnected'


class LogItem_GPS_On(LogItem):
    tag = 0x0E
    name = 'GPSOn'


class LogItem_GPS_Off(LogItem):
    tag = 0x0F
    name = 'GPSOff'


class LogItem_SoftWatchdog(LogItem):
    tag = 0x10
    name = 'SoftWatchdog'

    fields = [ 'watchdogAddress' ]

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'I', self.fields, **kwargs)

    def unpack(self, data):
        LogItem.unpack(self, data)
        self.watchdogAddress = hex(self.watchdogAddress)


class LogItem_Startup(LogItem):
    tag = 0x11
    name = 'Startup'

    fields = [ 'cause' ]

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'I', self.fields, **kwargs)

    def unpack(self, data):
        LogItem.unpack(self, data)
        self.cause = hex(self.cause)


class LogItem_IOT_Status(LogItem):
    tag = 0x20
    name = 'IOTStatus'
    fields = ['status']
    allowed_status = ['CELLULAR_POWERED_OFF', 'CELLULAR_POWERED_ON', 'CELLULAR_CONNECT',
                      'CELLULAR_FETCH_DEVICE_SHADOW', 'CELLULAR_SEND_LOGGING', 'CELLULAR_SEND_DEVICE_STATUS',
                      'CELLULAR_MAX_BACKOFF_REACHED', 'CELLULAR_DOWNLOAD_FIRMWARE', 'CELLULAR_DOWNLOAD_CONFIG',
                      'SATELLITE_POWERED_OFF', 'SATELLITE_POWERED_ON', 'SATELLITE_SEND_DEVICE_STATUS'
                      ]

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'B', self.fields, **kwargs)

    def pack(self):
        status = self.status
        if status in self.allowed_status:
            self.status = self.allowed_status.index(status)
        else:
            raise ExceptionLogInvalidValue('%s.status must be one of %s', self.name, self.allowed_status)
        data = LogItem.pack(self)
        self.status = status
        return data

    def unpack(self, data):
        LogItem.unpack(self, data)
        try:
            self.status = self.allowed_status[self.status]
        except:
            self.status = 'UNKNOWN'


class LogItem_IOT_ConfigUpdate(LogItem):
    tag = 0x21
    name = 'IOTConfigUpdate'
    fields = [ 'version', 'file_length' ]

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'II', self.fields, **kwargs)


class LogItem_IOT_FirmwareUpdate(LogItem):
    tag = 0x22
    name = 'IOTFirmwareUpdate'
    fields = [ 'version', 'file_length' ]

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'II', self.fields, **kwargs)


class LogItem_IOT_ErrorCode(LogItem):
    tag = 0x23
    name = 'IOTErrorCode'
    fields = [ 'iot_error_code', 'hal_error_code', 'hal_line_number', 'vendor_error_code']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'hhHH', self.fields, **kwargs)


class LogItem_IOT_NetworkInfo(LogItem):
    tag = 0x24
    name = 'IOTNetworkInfo'
    fields = [ 'signal_power', 'quality', 'technology', 'network_operator', 'location_area_code', 'cell_id']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'BBB25s5s9s', self.fields, **kwargs)

    def unpack(self, data):
        LogItem.unpack(self, data)
        self.network_operator = self.network_operator.decode('latin1').split('\0')[0]
        self.location_area_code = self.location_area_code.decode('latin1').split('\0')[0]
        self.cell_id = self.cell_id.decode('latin1').split('\0')[0]
        if self.technology == 0:
            self.technology = '2G'
        if self.technology == 2:
            self.technology = '3G'


class LogItem_IOT_Next_Prepas(LogItem):
    tag = 0x25
    name = 'IOTNextPrepas'
    fields = [ 'next_satellite_predict', 'gps_timestamp']

    def __init__(self, **kwargs):
        LogItem.__init__(self, b'II', self.fields, **kwargs)

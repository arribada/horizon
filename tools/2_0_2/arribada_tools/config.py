import struct
import logging
import sys
import json
import inspect
import binascii
import datetime
import dateutil.parser


__version__ = 8

logger = logging.getLogger(__name__)


class ExceptionConfigInvalidValue(Exception):
    pass


class ExceptionConfigInvalidJSONField(Exception):
    pass


def _flatdict(base, olddict, newdict):
    """Convert a JSON dict to a flat dict i.e., nested dictionaries
    are assigned using dotted notation to represent hierarchy e.g.,
    bluetooth.advertising"""
    for i in list(olddict.keys()):
        if isinstance(olddict[i], dict):
            _flatdict(base + ('.' if base else '') + i, olddict[i], newdict)
        else:
            newdict[base + ('.' if base else '') + i] = olddict[i]
    return newdict


def _pathsplit(fullpath):
    """Splits out the fullpath into a tuple comprising the variable name
    (i.e., given x.y.z, this would be z) and the root path (i.e., would
    be x.y given the previous example)"""
    items = fullpath.split('.')
    return ('.'.join(items[:-1]), items[-1])


def _findclass(fullpath):
    """Give a path name we identify to which configuration class it
    belongs.  The path uniquely identifies every configuration value
    and the root path denotes which class it belongs i.e., this code 
    performs a reverse search across all classes."""
    (path, param) = _pathsplit(fullpath)
    for i in inspect.getmembers(sys.modules[__name__], inspect.isclass):
        cls = i[1]
        if issubclass(cls, ConfigItem) and cls != ConfigItem and path == cls.path and \
            param in cls.json_params:
            return (path, param, cls)
    return (None, None, None)


def decode(data):
    """Attempt to decode a single configuration item from an input data buffer.
    A tuple is returned containing an instance of the configuration object plus
    the input data buffer, less the amount of data consumed."""
    item = TaggedItem()
    if (len(data) < item.header_length):
        return (None, data)

    # Unpack header tag at current position
    item.unpack(data)

    # Find the correct configuration class based on the configuration tag
    cfg = None
    for i in inspect.getmembers(sys.modules[__name__], inspect.isclass):
        cls = i[1]
        if issubclass(cls, ConfigItem) and cls != ConfigItem and \
            item.tag == cls.tag:
            cfg = cls()
            break
    if (cfg):
        try:
            cfg.unpack(data)
        except:
            # Likely insufficient bytes to unpack
            return (None, data)
        # Advance buffer past this configuration item
        data = data[cfg.length:]

    return (cfg, data)


def decode_all(data):
    """Iteratively decode an input data buffer to a list of configuration
    objects.
    """
    objects = []
    while data:
        (cfg, data) = decode(data)
        if cfg:
            objects += [ cfg ]
        else:
            break
    return objects


def encode_all(objects):
    """Encode a list of configuration objects, in order, to a serial byte
    stream.
    """
    data = b''
    for i in objects:
        data += i.pack()
    return data


def json_dumps(objects):
    """Convert a list of configuration objects representing a configuration
    set to JSON format."""
    obj_hash = {}
    for i in objects:
        h = obj_hash
        if i.path:
            p = i.path.split('.')
            for j in p:
                if j not in h:
                    h[j] = {}
                h = h[j]
        for j in i.json_params:
            h[j] = getattr(i, j)
    return json.dumps(obj_hash, indent=4, sort_keys=True)


def json_loads(text):
    """Convert JSON text representing a configuration set to a list
    of configuration objects."""
    obj = {}
    flat = _flatdict('', json.loads(text), {})
    for i in flat:
        (path, param, cls) = _findclass(i)
        if cls:
            if len(cls.params) == 1:
                path = i
            if path not in obj:
                obj[path] = cls()
            setattr(obj[path], param, flat[i])
        else:
            raise ExceptionConfigInvalidJSONField('Could not find "%s"' % i)
    return [obj[i] for i in obj]


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
            s += i + ' = ' + str(getattr(self, i, None)) + '\n'
        return s


class TaggedItem(_Blob):
    """Configuration item bare (without any value)"""
    def __init__(self, bytes_to_follow=0):
        _Blob.__init__(self, b'<H', ['tag'])
        self.header_length = struct.calcsize(self._fmt)
        self.length = self.header_length + bytes_to_follow


class ConfigItem(TaggedItem):
    """A configuration item which should be subclassed"""
    def __init__(self, fmt=b'', args=[], **kwargs):
        TaggedItem.__init__(self, struct.calcsize(b'<' + fmt))
        self.extend(fmt, args)
        for k in list(kwargs.keys()):
            setattr(self, k, kwargs[k])


class ConfigItem_Version(ConfigItem):
    tag = 0x0B00
    path = ''
    params = ['version']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'I', self.params, **kwargs)


class ConfigItem_System_DeviceName(ConfigItem):
    tag = 0x0400
    path = 'system'
    params = ['deviceName']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'256s', self.params, **kwargs)

    def pack(self):
        
        if hasattr(self, 'deviceName'):
            deviceName = self.deviceName
            if len(self.deviceName.encode('ascii', 'ignore')) > 255: # we use 255 bytes as the last must be a null '\0'
                raise ExceptionConfigInvalidValue('deviceName length must not exceed 255 bytes')
            self.deviceName = self.deviceName.encode('ascii', 'ignore') # we use 255 bytes as the last must be a null '\0'
        else:
            raise ExceptionConfigInvalidValue('deviceName is a mandatory parameter')

        data = ConfigItem.pack(self)
        self.deviceName = deviceName
        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        self.deviceName = self.deviceName.decode('ascii').rstrip('\x00') # Remove null characters


class ConfigItem_GPS_LogPositionEnable(ConfigItem):
    tag = 0x0000
    path = 'gps'
    params = ['logPositionEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_GPS_LogTTFFEnable(ConfigItem):
    tag = 0x0001
    path = 'gps'
    params = ['logTTFFEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_GPS_Mode(ConfigItem):
    tag = 0x0002
    path = 'gps'
    params = ['mode']
    json_params = params
    allowed_mode = [ 'SCHEDULED', 'SALTWATER_SWITCH', 'ACCELEROMETER', 'REED_SWITCH' ]

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'B', self.params, **kwargs)

    def pack(self):
        mode = self.mode
        try:
            self.mode = 0
            for i in mode:
                if i.upper() not in self.allowed_mode:
                    raise ExceptionConfigInvalidValue('mode must be one of %s' % self.allowed_mode)
                self.mode |= (1<<self.allowed_mode.index(i.upper()))
            data = ConfigItem.pack(self)
            return data
        finally:
            self.mode = mode

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        mode = self.mode
        self.mode = []
        for i in range(len(self.allowed_mode)):
            if mode & (1<<i):
                self.mode.append(self.allowed_mode[i])


class ConfigItem_GPS_ScheduledAquisitionInterval(ConfigItem):
    tag = 0x0003
    path = 'gps'
    params = ['scheduledAquisitionInterval']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_GPS_MaximumAquisitionTime(ConfigItem):
    tag = 0x0004
    path = 'gps'
    params = ['maximumAquisitionTime']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_GPS_ScheduledAquisitionNoFixTimeout(ConfigItem):
    tag = 0x0005
    path = 'gps'
    params = ['scheduledAquisitionNoFixTimeout']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_GPS_LastKnownPosition(ConfigItem):
    tag = 0x0006
    path = 'gps.lastKnownPosition'
    params = ['iTOW', 'longitude', 'latitude', 'height', 'accuracyHorizontal', 'accuracyVertical', 'year', 'month', 'day', 'hours', 'minutes', 'seconds']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'IlllIIH5B', self.params, **kwargs)

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
        data = ConfigItem.pack(self)
        self.longitude = longitude
        self.latitude = latitude
        self.height = height
        self.accuracyHorizontal = accuracyHorizontal
        self.accuracyVertical = accuracyVertical
        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        self.longitude = 1E-7 * self.longitude
        self.latitude = 1E-7 * self.latitude
        self.height = self.height / 1000.0
        self.accuracyHorizontal = self.accuracyHorizontal / 1000.0
        self.accuracyVertical = self.accuracyVertical / 1000.0


class ConfigItem_GPS_TestFixHoldTime(ConfigItem):
    tag = 0x0007
    path = 'gps'
    params = ['testFixHoldTime']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_GPS_LogDebugEnable(ConfigItem):
    tag = 0x0008
    path = 'gps'
    params = ['logDebugEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_GPS_MaxFixes(ConfigItem):
    tag = 0x0009
    path = 'gps'
    params = ['maxFixes']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'B', self.params, **kwargs)


class ConfigItem_SaltwaterSwitch_LogEnable(ConfigItem):
    tag = 0x0800
    path = 'saltwaterSwitch'
    params = ['logEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_SaltwaterSwitch_HysteresisPeriod(ConfigItem):
    tag = 0x0801
    path = 'saltwaterSwitch'
    params = ['hysteresisPeriod']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_RTC_SyncToGPSEnable(ConfigItem):
    tag = 0x0600
    path = 'rtc'
    params = ['syncToGPSEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_RTC_CurrentDateTime(ConfigItem):
    tag = 0x0601
    path = 'rtc'
    params = ['day', 'month', 'year', 'hours', 'minutes', 'seconds']
    json_params = ['dateTime']

    def __init__(self, **kwargs):
        if 'dateTime' in kwargs:
            self.dateTime = kwargs['dateTime']
            del kwargs['dateTime']
        else:
            self.dateTime = None
        ConfigItem.__init__(self, b'BBHBBB', self.params, **kwargs)

    def pack(self):
        if self.dateTime:
            d = dateutil.parser.parse(self.dateTime)
            self.day = d.day
            self.month = d.month
            self.year = d.year
            self.hours = d.hour
            self.minutes = d.minute
            self.seconds = d.second
        return ConfigItem.pack(self)

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        d = datetime.datetime(day=self.day,
                     month=self.month,
                     year=self.year,
                     hour=self.hours,
                     minute=self.minutes,
                     second=self.seconds)
        self.dateTime = d.ctime()


class ConfigItem_Logging_Enable(ConfigItem):
    tag = 0x0100
    path = 'logging'
    params = ['enable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_Logging_FileSize(ConfigItem):
    tag = 0x0101
    path = 'logging'
    params = ['fileSize']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'L', self.params, **kwargs)


class ConfigItem_Logging_FileType(ConfigItem):
    tag = 0x0102
    path = 'logging'
    params = ['fileType']
    json_params = params
    allowed_file_type = ['LINEAR', 'CIRCULAR']

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'B', self.params, **kwargs)

    def pack(self):
        fileType = self.fileType
        if fileType in self.allowed_file_type:
            self.fileType = self.allowed_file_type.index(fileType)
        else:
            raise ExceptionConfigInvalidValue('fileType must be one of %s' % self.allowed_file_type)

        data = ConfigItem.pack(self)
        self.fileType = fileType
        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        try:
            self.fileType = self.allowed_file_type[self.fileType]
        except:
            self.fileType = 'UNKNOWN'


class ConfigItem_Logging_GroupSensorReadingsEnable(ConfigItem):
    tag = 0x0103
    path = 'logging'
    params = ['groupSensorReadingsEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_Logging_StartEndSyncEnable(ConfigItem):
    tag = 0x0104
    path = 'logging'
    params = ['startEndSyncEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_Logging_DateTimeStampEnable(ConfigItem):
    tag = 0x0105
    path = 'logging'
    params = ['dateTimeStampEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_Logging_HighResolutionTimerEnable(ConfigItem):
    tag = 0x0106
    path = 'logging'
    params = ['highResolutionTimerEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_AXL_LogEnable(ConfigItem):
    tag = 0x0200
    path = 'accelerometer'
    params = ['logEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_AXL_Config(ConfigItem):
    tag = 0x0201
    path = 'accelerometer'
    params = ['config']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'B', self.params, **kwargs)


class ConfigItem_AXL_HighThreshold(ConfigItem):
    tag = 0x0202
    path = 'accelerometer'
    params = ['highThreshold']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_AXL_SampleRate(ConfigItem):
    tag = 0x0203
    path = 'accelerometer'
    params = ['sampleRate']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_AXL_Mode(ConfigItem):
    tag = 0x0204
    path = 'accelerometer'
    params = ['mode']
    json_params = params
    allowed_mode = ['PERIODIC', 'TRIGGER_ABOVE']

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'B', self.params, **kwargs)

    def pack(self):
        mode = self.mode
        if mode in self.allowed_mode:
            self.mode = self.allowed_mode.index(mode)
        else:
            raise ExceptionConfigInvalidValue('mode must be one of %s' % self.allowed_mode)

        data = ConfigItem.pack(self)
        self.mode = mode
        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        try:
            self.mode = self.allowed_mode[self.mode]
        except:
            self.mode = 'UNKNOWN'


class ConfigItem_AXL_ScheduledAquisitionInterval(ConfigItem):
    tag = 0x0205
    path = 'accelerometer'
    params = ['scheduledAquisitionInterval']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_AXL_MaximumAquisitionTime(ConfigItem):
    tag = 0x0206
    path = 'accelerometer'
    params = ['maximumAquisitionTime']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_PressureSensor_LogEnable(ConfigItem):
    tag = 0x0300
    path = 'pressureSensor'
    params = ['logEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_PressureSensor_SampleRate(ConfigItem):
    tag = 0x0301
    path = 'pressureSensor'
    params = ['sampleRate']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_PressureSensor_LowThreshold(ConfigItem):
    tag = 0x0302
    path = 'pressureSensor'
    params = ['lowThreshold']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_PressureSensor_HighThreshold(ConfigItem):
    tag = 0x0303
    path = 'pressureSensor'
    params = ['highThreshold']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_PressureSensor_Mode(ConfigItem):
    tag = 0x0304
    path = 'pressureSensor'
    params = ['mode']
    json_params = params
    allowed_mode = ['PERIODIC', 'TRIGGER_BELOW', 'TRIGGER_BETWEEN', 'TRIGGER_ABOVE']

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'B', self.params, **kwargs)

    def pack(self):
        mode = self.mode
        if mode in self.allowed_mode:
            self.mode = self.allowed_mode.index(mode)
        else:
            raise ExceptionConfigInvalidValue('mode must be one of %s' % self.allowed_mode)

        data = ConfigItem.pack(self)
        self.mode = mode
        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        try:
            self.mode = self.allowed_mode[self.mode]
        except:
            self.mode = 'UNKNOWN'


class ConfigItem_PressureSensor_ScheduledAquisitionInterval(ConfigItem):
    tag = 0x0305
    path = 'pressureSensor'
    params = ['scheduledAquisitionInterval']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_PressureSensor_MaximumAquisitionTime(ConfigItem):
    tag = 0x0306
    path = 'pressureSensor'
    params = ['maximumAquisitionTime']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_TempSensor_LogEnable(ConfigItem):
    tag = 0x0700
    path = 'temperateSensor'
    params = ['logEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_TempSensor_SampleRate(ConfigItem):
    tag = 0x0701
    path = 'temperateSensor'
    params = ['sampleRate']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_TempSensor_LowThreshold(ConfigItem):
    tag = 0x0702
    path = 'temperateSensor'
    params = ['lowThreshold']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_TempSensor_HighThreshold(ConfigItem):
    tag = 0x0703
    path = 'temperateSensor'
    params = ['highThreshold']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_TempSensor_Mode(ConfigItem):
    tag = 0x0704
    path = 'temperateSensor'
    params = ['mode']
    json_params = params
    allowed_mode = ['PERIODIC', 'TRIGGER_BELOW', 'TRIGGER_BETWEEN', 'TRIGGER_ABOVE']

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'B', self.params, **kwargs)

    def pack(self):
        mode = self.mode
        if mode in self.allowed_mode:
            self.mode = self.allowed_mode.index(mode)
        else:
            raise ExceptionConfigInvalidValue('mode must be one of %s' % self.allowed_mode)

        data = ConfigItem.pack(self)
        self.mode = mode
        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        try:
            self.mode = self.allowed_mode[self.mode]
        except:
            self.mode = 'UNKNOWN'


class ConfigItem_BLE_DeviceAddress(ConfigItem):
    tag = 0x0500
    path = 'bluetooth'
    params = ['deviceAddress']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'6s', self.params, **kwargs)

    def pack(self):
        old = self.deviceAddress
        self.deviceAddress = binascii.unhexlify(self.deviceAddress.replace(':', ''))[::-1]
        if (ord(self.deviceAddress[5]) & 0b11000000) != 0b11000000:
            raise ExceptionConfigInvalidValue('deviceAddress must have top two bits set')
        data = ConfigItem.pack(self)
        self.deviceAddress = old
        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        device_id = binascii.hexlify(self.deviceAddress[::-1])
        new_id = ""
        for i in range(len(device_id)):
            new_id = new_id + chr(device_id[i]).upper()
            if i & 1 and i != (len(device_id)-1):
                new_id = new_id + ':'
        self.deviceAddress = new_id


class ConfigItem_BLE_TriggerControl(ConfigItem):
    tag = 0x0501
    path = 'bluetooth'
    params = ['triggerControl']
    json_params = params
    allowed_trigger_control = ['REED_SWITCH', 'SCHEDULED', 'GEOFENCE', 'ONE_SHOT']

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'B', self.params, **kwargs)

    def pack(self):
        if hasattr(self, 'triggerControl'):
            triggerControl = self.triggerControl
            self.triggerControl = 0
            for i in triggerControl:
                if i.upper() in self.allowed_trigger_control:
                    self.triggerControl = self.triggerControl | (1 << self.allowed_trigger_control.index(i.upper()))
                else:
                    raise ExceptionConfigInvalidValue('triggerControl must be one of %s' % self.allowed_trigger_control)
        else:
            triggerControl = []
            self.triggerControl = 0

        data = ConfigItem.pack(self)
        self.triggerControl = triggerControl
        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        triggerControl = []
        for i in self.allowed_trigger_control:
            if self.triggerControl & (1<<self.allowed_trigger_control.index(i)):
                triggerControl.append(i)
        self.triggerControl = triggerControl


class ConfigItem_BLE_ScheduledInterval(ConfigItem):
    tag = 0x0502
    path = 'bluetooth'
    params = ['scheduledInterval']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_BLE_ScheduledDuration(ConfigItem):
    tag = 0x0503
    path = 'bluetooth'
    params = ['scheduledDuration']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_BLE_AdvertisingInterval(ConfigItem):
    tag = 0x0504
    path = 'bluetooth'
    params = ['advertisingInterval']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_BLE_ConnectionInterval(ConfigItem):
    tag = 0x0505
    path = 'bluetooth'
    params = ['connectionInterval']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_BLE_InactivityTimeout(ConfigItem):
    tag = 0x0506
    path = 'bluetooth'
    params = ['inactivityTimeout']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'H', self.params, **kwargs)


class ConfigItem_BLE_PHYMode(ConfigItem):
    tag = 0x0507
    path = 'bluetooth'
    params = ['phyMode']
    json_params = params
    allowed_mode = ['1_MBPS', '2_MBPS']

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'B', self.params, **kwargs)

    def pack(self):
        phyMode = self.phyMode
        if phyMode in self.allowed_mode:
            self.phyMode = self.allowed_mode.index(phyMode)
        else:
            raise ExceptionConfigInvalidValue('phyMode must be one of %s' % self.allowed_mode)

        data = ConfigItem.pack(self)
        self.phyMode = phyMode
        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        try:
            self.phyMode = self.allowed_mode[self.phyMode]
        except:
            self.phyMode = 'UNKNOWN'


class ConfigItem_BLE_LogEnable(ConfigItem):
    tag = 0x0508
    path = 'bluetooth'
    params = ['logEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_BLE_AdvertisingTags(ConfigItem):
    tag = 0x0509
    path = 'bluetooth'
    params = ['advertisingTags']
    json_params = params
    allowed_tags = ['LAST_LOG_READ_POS', 'LAST_GPS_LOCATION', 'BATTERY_LEVEL',
                    'BATTERY_VOLTAGE', 'LAST_CELLULAR_CONNECT', 'LAST_SAT_TX',
                    'NEXT_SAT_TX', 'CONFIG_VERSION', 'FW_VERSION']

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'I', self.params, **kwargs)

    def pack(self):
        if hasattr(self, 'advertisingTags'):
            advertisingTags = self.advertisingTags
            self.advertisingTags = 0
            for i in advertisingTags:
                if i in self.allowed_tags:
                    self.advertisingTags = self.advertisingTags | (1 << self.allowed_tags.index(i))
                else:
                    raise ExceptionConfigInvalidValue('advertisingTags must be one of %s' % self.allowed_tags)
        else:
            advertisingTags = []
            self.advertisingTags = 0

        data = ConfigItem.pack(self)
        self.advertisingTags = advertisingTags
        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        advertisingTags = []
        for i in self.allowed_tags:
            if self.advertisingTags & (1<<self.allowed_tags.index(i)):
                advertisingTags.append(i)
        self.advertisingTags = advertisingTags


class ConfigItem_IOT_General(ConfigItem):
    tag = 0x0A00
    path = 'iot'
    params = ['enable', 'logEnable', 'minBatteryThreshold']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'??B', self.params, **kwargs)
        if not hasattr(self, 'enable'):
            self.enable = True
        if not hasattr(self, 'minBatteryThreshold'):
            self.minBatteryThreshold = 0
        if not hasattr(self, 'logEnable'):
            self.logEnable = False

    def pack(self):
        if self.minBatteryThreshold > 100 or self.minBatteryThreshold < 0:
            raise ExceptionConfigInvalidValue('minBatteryThreshold must be between 0..100')
        data = ConfigItem.pack(self)
        return data


class ConfigItem_IOT_Cellular(ConfigItem):
    tag = 0x0A01
    path = 'iot.cellular'
    params = ['enable', 'connectionPriority', 'connectionMode', 'logFilter',
              'statusFilter', 'checkFirmwareUpdates', 'checkConfigurationUpdates',
              'minUpdates', 'maxInterval', 'minInterval', 'maxBackoffInterval',
              'gpsScheduleIntervalOnMaxBackoff']
    json_params = params
    allow_connection_mode = ['2G', '3G', 'AUTO']

    # The order of these allowed filter options must match the bit-field order as
    # implemented by the embedded software
    allowed_log_filter = ['GPS', 'TIMESTAMP', 'DATETIME', 'BATTERY_VOLTAGE', 'BATTERY_LEVEL' ]
    allowed_status_filter = ['LAST_LOG_READ_POS', 'LAST_GPS_LOCATION', 'BATTERY_LEVEL',
                             'BATTERY_VOLTAGE', 'LAST_CELLULAR_CONNECT', 'LAST_SAT_TX',
                             'NEXT_SAT_TX', 'CONFIG_VERSION', 'FW_VERSION']                             

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?BBII??BIIII', self.params, **kwargs)
        if not hasattr(self, 'enable'):
            self.enable = True
        if not hasattr(self, 'connectionMode'):
            self.connectionMode = 'AUTO'
        if not hasattr(self, 'connectionPriority'):
            self.connectionPriority = 0
        if not hasattr(self, 'logFilter'):
            self.logFilter = []
        if not hasattr(self, 'checkFirmwareUpdates'):
            self.checkFirmwareUpdates = True
        if not hasattr(self, 'checkConfigurationUpdates'):
            self.checkConfigurationUpdates = True
        if not hasattr(self, 'minUpdates'):
            self.minUpdates = 1
        if not hasattr(self, 'maxInterval'):
            self.maxInterval = 0     # Means disable
        if not hasattr(self, 'minInterval'):
            self.minInterval = 0     # Means disable
        if not hasattr(self, 'maxBackoffInterval'):
            self.maxBackoffInterval = 24*60*60
        if not hasattr(self, 'gpsScheduleIntervalOnMaxBackoff'):
            self.gpsScheduleIntervalOnMaxBackoff = 0     # Means disable

    def pack(self):
        
        connectionMode = self.connectionMode
        if connectionMode in self.allow_connection_mode:
            self.connectionMode = self.allow_connection_mode.index(connectionMode)
        else:
            raise ExceptionConfigInvalidValue('connectionMode must be one of %s' % self.allow_connection_mode)

        if self.connectionPriority > 10 or self.connectionPriority < 0:
            raise ExceptionConfigInvalidValue('connectionPriority must be between 0..10')

        logFilter = self.logFilter
        self.logFilter = 0
        for i in logFilter:
            if i in self.allowed_log_filter:
                self.logFilter = self.logFilter | (1 << self.allowed_log_filter.index(i))
            else:
                raise ExceptionConfigInvalidValue('logFilter must be one of %s' % self.allowed_log_filter)

        if hasattr(self, 'statusFilter'):
            statusFilter = self.statusFilter
            self.statusFilter = 0
            for i in statusFilter:
                if i in self.allowed_status_filter:
                    self.statusFilter = self.statusFilter | (1 << self.allowed_status_filter.index(i))
                else:
                    raise ExceptionConfigInvalidValue('statusFilter must be one of %s' % self.allowed_status_filter)
            if self.logFilter > 0 and not self.statusFilter & (1 << self.allowed_status_filter.index('LAST_LOG_READ_POS')):
                raise ExceptionConfigInvalidValue('statusFilter must contain LAST_LOG_READ_POS if logging (logFilter) is enabled')
        else:
            raise ExceptionConfigInvalidValue('statusFilter is a mandatory parameter')

        if self.minUpdates < 0:
            raise ExceptionConfigInvalidValue('minUpdates must be >= 0')

        data = ConfigItem.pack(self)
        self.connectionMode = connectionMode
        self.logFilter = logFilter
        self.statusFilter = statusFilter

        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        try:
            self.connectionMode = self.allow_connection_mode[self.connectionMode]
        except:
            self.connectionMode = 'UNKNOWN'

        statusFilter = []
        for i in self.allowed_status_filter:
            if self.statusFilter & (1<<self.allowed_status_filter.index(i)):
                statusFilter.append(i)
        self.statusFilter = statusFilter

        logFilter = []
        for i in self.allowed_log_filter:
            if self.logFilter & (1<<self.allowed_log_filter.index(i)):
                logFilter.append(i)
        self.logFilter = logFilter


class ConfigItem_IOT_Cellular_AWS(ConfigItem):
    tag = 0x0A02
    path = 'iot.cellular.aws'
    params = ['arn', 'port', 'thingName', 'loggingTopicPath', 'deviceShadowPath']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'64sH256s256s256s', self.params, **kwargs)
        if not hasattr(self, 'port'):
            self.port = 8443
        if not hasattr(self, 'thingName'):
            self.thingName = b''   # Means use deviceName instead
        if not hasattr(self, 'loggingTopicPath'):
            self.loggingTopicPath = '/topics/#/logging'
        if not hasattr(self, 'deviceShadowPath'):
            self.deviceShadowPath = '/things/#/shadow'

    def pack(self):

        if not hasattr(self, 'arn'):
            raise ExceptionConfigInvalidValue('arn is a mandatory parameter')

        if len(self.arn.encode('ascii', 'ignore')) > 63: # we use 63 bytes as the last must be a null '\0'
            raise ExceptionConfigInvalidValue('arn length must not exceed 63 bytes')
        arn = self.arn
        self.arn = self.arn.encode('ascii', 'ignore')

        if len(self.thingName.encode('ascii', 'ignore')) > 255: # we use 255 bytes as the last must be a null '\0'
            raise ExceptionConfigInvalidValue('thingName must not exceed 255 bytes')
        thingName = self.thingName
        self.thingName = self.thingName.encode('ascii', 'ignore')

        if len(self.loggingTopicPath.encode('ascii', 'ignore')) > 255: # we use 255 bytes as the last must be a null '\0'
            raise ExceptionConfigInvalidValue('loggingTopicPath length must not exceed 255 bytes')
        loggingTopicPath = self.loggingTopicPath
        self.loggingTopicPath = self.loggingTopicPath.encode('ascii', 'ignore')

        if len(self.deviceShadowPath.encode('ascii', 'ignore')) > 255: # we use 255 bytes as the last must be a null '\0'
            raise ExceptionConfigInvalidValue('deviceShadowPath must not exceed 255 bytes')
        deviceShadowPath = self.deviceShadowPath
        self.deviceShadowPath = self.deviceShadowPath.encode('ascii', 'ignore')

        data = ConfigItem.pack(self)

        self.arn = arn
        self.thingName = thingName
        self.loggingTopicPath = loggingTopicPath
        self.deviceShadowPath = deviceShadowPath
        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        # Remove null characters
        self.arn = self.arn.decode('ascii').rstrip('\x00')
        self.thingName = self.thingName.decode('ascii').rstrip('\x00')
        self.loggingTopicPath = self.loggingTopicPath.decode('ascii').rstrip('\x00')
        self.deviceShadowPath = self.deviceShadowPath.decode('ascii').rstrip('\x00')



class ConfigItem_IOT_Cellular_APN(ConfigItem):
    tag = 0x0A03
    path = 'iot.cellular.apn'
    params = ['name', 'username', 'password']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'128s32s32s', self.params, **kwargs)
        if not hasattr(self, 'username'):
            self.username = ''
        if not hasattr(self, 'password'):
            self.password = ''

    def pack(self):

        if not hasattr(self, 'name'):
            raise ExceptionConfigInvalidValue('name is a mandatory parameter')

        if len(self.name.encode('ascii', 'ignore')) > 127: # N-1 the last must be a null '\0'
            raise ExceptionConfigInvalidValue('name length must not exceed 127 bytes')
        name = self.name
        self.name = self.name.encode('ascii', 'ignore')

        if len(self.username.encode('ascii', 'ignore')) > 31: # N-1 the last must be a null '\0'
            raise ExceptionConfigInvalidValue('username must not exceed 31 bytes')
        username = self.username
        self.username = self.username.encode('ascii', 'ignore')

        if len(self.password.encode('ascii', 'ignore')) > 31: # N-1 the last must be a null '\0'
            raise ExceptionConfigInvalidValue('password length must not exceed 31 bytes')
        password = self.password
        self.password = self.password.encode('ascii', 'ignore')

        data = ConfigItem.pack(self)

        self.name = name
        self.username = username
        self.password = password
        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)
        # Remove null characters
        self.name = self.name.decode('ascii').rstrip('\x00')
        self.username = self.username.decode('ascii').rstrip('\x00')
        self.password = self.password.decode('ascii').rstrip('\x00')


class ConfigItem_IOT_Satellite(ConfigItem):
    tag = 0x0A10
    path = 'iot.satellite'
    params = ['enable', 'connectionPriority', 'statusFilter', 'minUpdates',
              'maxInterval', 'minInterval', 'randomizedTxWindow', 'testModeEnable', 'retransmissionCount', 'retransmissionInterval']
    json_params = params

    # The order of these allowed filter options must match the bit-field order as
    # implemented by the embedded software
    allowed_status_filter = ['LAST_LOG_READ_POS', 'LAST_GPS_LOCATION', 'BATTERY_LEVEL',
                             'BATTERY_VOLTAGE', 'LAST_CELLULAR_CONNECT', 'LAST_SAT_TX',
                             'NEXT_SAT_TX', 'CONFIG_VERSION', 'FW_VERSION']
    # Maximum length permitted for Artic modem is 31 bytes for status reports - track how
    # many bytes are needed based on statusFilter configuration
    status_max_length = 31
    status_header_length = 4
    status_filter_length = {
        'LAST_LOG_READ_POS': 4,
        'LAST_GPS_LOCATION': 12,
        'BATTERY_LEVEL': 1,
        'BATTERY_VOLTAGE': 2,
        'LAST_CELLULAR_CONNECT': 4,
        'LAST_SAT_TX': 4,
        'NEXT_SAT_TX': 4,
        'CONFIG_VERSION': 4,
        'FW_VERSION': 4
    }

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?BIBIIB?HH', self.params, **kwargs)
        if not hasattr(self, 'enable'):
            self.enable = True
        if not hasattr(self, 'connectionPriority'):
            self.connectionPriority = 0
        if not hasattr(self, 'minUpdates'):
            self.minUpdates = 1
        if not hasattr(self, 'maxInterval'):
            self.maxInterval = 0     # Means disable
        if not hasattr(self, 'minInterval'):
            self.minInterval = 0     # Means disable
        if not hasattr(self, 'randomizedTxWindow'):
            self.randomizedTxWindow = 0
        if not hasattr(self, 'testModeEnable'):
            self.testModeEnable = 0     # Means disable
        if not hasattr(self, 'retransmissionCount'):
            self.retransmissionCount = 0     # Means disable
        if not hasattr(self, 'retransmissionInterval'):
            self.retransmissionInterval = 60

    def pack(self):
        
        if hasattr(self, 'statusFilter'):
            actual_length = self.status_header_length
            statusFilter = self.statusFilter
            self.statusFilter = 0
            for i in statusFilter:
                if i in self.allowed_status_filter:
                    self.statusFilter = self.statusFilter | (1 << self.allowed_status_filter.index(i))
                    actual_length += self.status_filter_length[i]
                else:
                    raise ExceptionConfigInvalidValue('statusFilter must be one of %s' % self.allowed_status_filter)
                if actual_length > self.status_max_length:
                    raise ExceptionConfigInvalidValue('statusFilter requires %s bytes payload size versus max. %s' % (actual_length, self.status_max_length))
        else:
            raise ExceptionConfigInvalidValue('statusFilter is a mandatory parameter')

        if self.minUpdates < 0:
            raise ExceptionConfigInvalidValue('minUpdates must be >= 0')

        if hasattr(self, 'retransmissionInterval'):
            if self.retransmissionInterval < 45:
                raise ExceptionConfigInvalidValue('retransmissionInterval "%s" is not valid - must be >= 45' % self.retransmissionInterval)

        data = ConfigItem.pack(self)
        self.statusFilter = statusFilter

        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)

        statusFilter = []
        for i in self.allowed_status_filter:
            if self.statusFilter & (1<<self.allowed_status_filter.index(i)):
                statusFilter.append(i)
        self.statusFilter = statusFilter


class ConfigItem_IOT_Satellite_Artic(ConfigItem):
    tag = 0x0A11
    path = 'iot.satellite.artic'
    params = ['deviceAddress', 'minElevation', 'warmupTime', 'argosProtocol', 'bulletin']
    json_params = params
    allowed_bulletin_fields = ['satelliteCode', 'secondsSinceEpoch', 'params']
    bulletin_raw = b''
    dev_addr = b''

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'4sBBB240s', self.params, **kwargs)
        if not hasattr(self, 'argosProtocol'):
            self.argosProtocol = 3

    def pack(self):

        if hasattr(self, 'deviceAddress'):
            dev_addr = binascii.unhexlify(self.deviceAddress.replace(':', ''))[::-1]
            if len(dev_addr) != 4:
                raise ExceptionConfigInvalidValue('deviceAddress must be 4 bytes long')
        else:
            raise ExceptionConfigInvalidValue('deviceAddress is a mandatory parameter')

        if hasattr(self, 'minElevation'):
            if self.minElevation < 0 or self.minElevation > 70:
                raise ExceptionConfigInvalidValue('minElevation "%s" is not valid - use range is 0..70' % self.minElevation)
        else:
            self.minElevation = 45
        
        if hasattr(self, 'argosProtocol'):
            if self.argosProtocol != 2 and self.argosProtocol != 3:
                raise ExceptionConfigInvalidValue('argosProtocol "%s" is not valid - must be either 2 or 3' % self.argosProtocol)

        if hasattr(self, 'warmupTime'):
            if self.warmupTime < 3 or self.warmupTime > 20:
                raise ExceptionConfigInvalidValue('warmupTime "%s" is not valid - use range is 3..20' % self.warmupTime)
        else:
            self.warmupTime = 3

        if hasattr(self, 'bulletin'):
            if type(self.bulletin) is not list:
                raise ExceptionConfigInvalidValue('bulletin is a mandatory parameter')
            if len(self.bulletin) == 0:
                logger.warn('bulletin is empty - prepass algorithm will be disabled on target')
            for i in self.bulletin:
                if type(i) is not dict:
                    raise ExceptionConfigInvalidValue('bulletin entry "%s" is not valid' % i)
                sat_code = None
                seconds_since_epoch = None
                bulletin_params = None
                for k in list(i.keys()):
                    if k == 'satelliteCode':
                        sat_code = i[k]
                        if len(sat_code) != 2:
                            raise ExceptionConfigInvalidValue('bulletin entry "%s" contains invalid fields - satelliteCode must be two ASCII bytes' % i)
                    elif k == 'secondsSinceEpoch':
                        seconds_since_epoch = i[k]
                    elif k == 'params':
                        bulletin_params = i[k]
                        if len(bulletin_params) != 6:
                            raise ExceptionConfigInvalidValue('bulletin entry "%s" contains invalid fields - params must be 6 floats' % i)
                    else:
                        raise ExceptionConfigInvalidValue('bulletin entry "%s" contains invalid fields - use: %s' % (i, self.allowed_bulletin_fields))
                if None in [sat_code, seconds_since_epoch, bulletin_params]:
                    raise ExceptionConfigInvalidValue('bulletin entry "%s" must specify all fields: %s' % (i, self.allowed_bulletin_fields))
                self.bulletin_raw += struct.pack(b'<2sI6f', sat_code.encode('utf-8'), seconds_since_epoch, *bulletin_params)
        else:
            raise ExceptionConfigInvalidValue('bulletin is a mandatory parameter')

        bulletin = self.bulletin
        self.bulletin = self.bulletin_raw
        addr = self.deviceAddress
        self.deviceAddress = dev_addr

        data = ConfigItem.pack(self)

        self.bulletin = bulletin
        self.deviceAddress = addr

        return data

    def unpack(self, data):
        ConfigItem.unpack(self, data)

        device_id = binascii.hexlify(self.deviceAddress[::-1])
        new_id = ""
        for i in range(len(device_id)):
            new_id = new_id + chr(device_id[i]).upper()
            if i & 1 and i != (len(device_id)-1):
                new_id = new_id + ':'
        self.deviceAddress = new_id

        bulletin_raw = self.bulletin
        fmt = b'<2sI6f'
        bulletin = []
        i = 0

        while i < len(bulletin_raw):
            (sat_code, timestamp, f1, f2, f3, f4, f5, f6) = struct.unpack_from(fmt, bulletin_raw, i)
            i = i + struct.calcsize(fmt)
            if sat_code[0] != 0:
                bulletin.append({'satelliteCode':sat_code.decode('ascii'),
                                 'secondsSinceEpoch':timestamp,
                                 'params': [f1,f2,f3,f4,f5,f6]})
            else:
                break
        
        self.bulletin = bulletin


class ConfigItem_Battery_LogEnable(ConfigItem):
    tag = 0x0900
    path = 'battery'
    params = ['logEnable']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'?', self.params, **kwargs)


class ConfigItem_Battery_LowThreshold(ConfigItem):
    tag = 0x0901
    path = 'battery'
    params = ['lowThreshold']
    json_params = params

    def __init__(self, **kwargs):
        ConfigItem.__init__(self, b'B', self.params, **kwargs)

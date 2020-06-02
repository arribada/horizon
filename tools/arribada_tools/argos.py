import struct
import logging
import sys
import inspect
import binascii

logger = logging.getLogger(__name__)


class ExceptionLogInvalidValue(Exception):
    pass


def decode(data):
    """Attempt to decode a single log item from an input data buffer.
    A tuple is returned containing an instance of the log object plus
    the input data buffer, less the amount of data consumed."""

    if not data or len(data) < 5:
        return None

    # Unpack header tag at current position
    unpacker = struct.Struct('<BI')
    presence_field = unpacker.unpack_from(data)
    packet_type = presence_field[0]

    if packet_type != 0:
        return None

    presence_field = presence_field[1]

    data = data[5:]

    # Find the correct configuration class based on the configuration tag
    contents = []
    for i in inspect.getmembers(sys.modules[__name__], inspect.isclass):
        cls = i[1]
        if issubclass(cls, ArgosItem) and cls != ArgosItem:
            if presence_field & (1 << cls.presence_flag_idx):
                contents.append(cls())
    if contents:
        contents.sort(key=lambda x: x.presence_flag_idx)
        for tag in contents:
            try:
                tag.unpack(data)
                data = data[tag._size:]
            except:
                break
        pass

    return contents


class TaggedItem(object):
    """Blob object is a container for arbitrary message fields which
    can be packed / unpacked using python struct"""
    _fmt = ''
    _size = 0
    _args = []

    def __init__(self, fmt, size, args):
        self._fmt = fmt
        self._size = size
        self._args = args

    def extend(self, fmt, args):
        self._fmt += fmt
        self._args += args

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


class ArgosItem(TaggedItem):
    """An argos item which should be subclassed"""
    def __init__(self, fmt=b'', args=[], **kwargs):
        format = b'<' + fmt
        TaggedItem.__init__(self, format, struct.calcsize(format), args)
        for k in list(kwargs.keys()):
            setattr(self, k, kwargs[k])


class ArgosItem_last_log_file_read_pos(ArgosItem):
    presence_flag_idx = 0
    name = 'LastLogFileReadPos'
    fields = ['lastPos']

    def __init__(self, **kwargs):
        ArgosItem.__init__(self, b'I', self.fields, **kwargs)


class ArgosItem_last_gps_location(ArgosItem):
    presence_flag_idx = 1
    name = 'LastGPSLocation'
    fields = ['longitude', 'latitude', 'timestamp']

    def __init__(self, **kwargs):
        ArgosItem.__init__(self, b'ffI', self.fields, **kwargs)

    def unpack(self, data):
        ArgosItem.unpack(self, data)
        self.longitude = 1E-7 * self.longitude
        self.latitude = 1E-7 * self.latitude


class ArgosItem_battery_level(ArgosItem):
    presence_flag_idx = 2
    name = 'BatteryLevel'
    fields = ['level']

    def __init__(self, **kwargs):
        ArgosItem.__init__(self, b'B', self.fields, **kwargs)


class ArgosItem_battery_voltage(ArgosItem):
    presence_flag_idx = 3
    name = 'BatteryVoltage'
    fields = ['voltage']

    def __init__(self, **kwargs):
        ArgosItem.__init__(self, b'H', self.fields, **kwargs)


class ArgosItem_last_cellular_connected_timestamp(ArgosItem):
    presence_flag_idx = 4
    name = 'LastCellularConnectedTimestamp'
    fields = ['timestamp']

    def __init__(self, **kwargs):
        ArgosItem.__init__(self, b'I', self.fields, **kwargs)


class ArgosItem_last_sat_tx_timestamp(ArgosItem):
    presence_flag_idx = 5
    name = 'LastSatelliteTransmissionTimestamp'
    fields = ['timestamp']

    def __init__(self, **kwargs):
        ArgosItem.__init__(self, b'I', self.fields, **kwargs)


class ArgosItem_next_sat_tx_timestamp(ArgosItem):
    presence_flag_idx = 6
    name = 'NextSatelliteTransmissionTimestamp'
    fields = ['timestamp']

    def __init__(self, **kwargs):
        ArgosItem.__init__(self, b'I', self.fields, **kwargs)


class ArgosItem_configuration_version(ArgosItem):
    presence_flag_idx = 7
    name = 'ConfigurationVersion'
    fields = ['version']

    def __init__(self, **kwargs):
        ArgosItem.__init__(self, b'I', self.fields, **kwargs)


class ArgosItem_firmware_version(ArgosItem):
    presence_flag_idx = 8
    name = 'FirmwareVersion'
    fields = ['version']

    def __init__(self, **kwargs):
        ArgosItem.__init__(self, b'I', self.fields, **kwargs)
import struct
import binascii
from array import array

_UBX_MIN_MESSAGE_LEN = 8


class _Sync(object):
    SYNC1 = b'\xb5'
    SYNC2 = b'\x62'


_ack_dict = {
    "class":"ACK",
    0x00:"NAK",
    0x01:"ACK"
}

_aid_dict = {
    "class":"AID",
    0x30:"ALM",
    0x33:"AOP",
    0x31:"EPH",
    0x02:"HUI",
    0x01:"INI",
}

_cfg_dict = {
    "class":"CFG",
    0x13:"ANT",
    0x09:"CFG",
    0x06:"DAT",
    0x61:"DOSC",
    0x85:"DYNSEED",
    0x60:"ESRC",
    0x84:"FIXSEED",
    0x69:"GEOFENCE",
    0x3E:"GNSS",
    0x02:"INF",
    0x39:"ITFM",
    0x47:"LOGFILTER",
    0x01:"MSG",
    0x24:"NAV5",
    0x23:"NAVX5",
    0x17:"NMEA",
    0x1E:"ODO",
    0x3B:"PM2",
    0x86:"PMS",
    0x00:"PRT",
    0x57:"PWR",
    0x08:"RATE",
    0x34:"RINV",
    0x04:"RST",
    0x11:"RXM",
    0x16:"SBAS",
    0x62:"SMGR",
    0x3D:"TMODE2",
    0x31:"TP5",
    0x53:"TXSLOT",
    0x1B:"USB",
}

_esf_dict = {
    "class":"ESF",
    0x10:"STATUS",
}

_inf_dict = {
    "class":"INF",
    0x04:"DEBUG",
    0x00:"ERROR",
    0x02:"NOTICE",
    0x03:"TEST",
    0x01:"WARNING",
}

_log_dict = {
    "class":"LOG",
    0x07:"CREATE",
    0x03:"ERASE",
    0x0E:"FINDTIME",
    0x08:"INFO",
    0x0f:"RETRIEVEPOSE...",
    0x0b:"RETRIEVEPOS",
    0x0d:"RETRIEVESTRING",
    0x09:"RETRIEVE",
    0x04:"STRING",
}

_mon_dict = {
    "class":"MON",
    0x28:"GNSS",
    0x0B:"HW2",
    0x09:"HW",
    0x02:"IO",
    0x06:"MSGPP",
    0x27:"PATCH",
    0x07:"RXBUF",
    0x21:"RXR",
    0x2E:"SMGR",
    0x08:"TXBUF",
    0x04:"VER",
}

_nav_dict = {
    "class":"NAV",
    0x60:"AOPSTATUS",
    0x22:"CLOCK",
    0x31:"DGPS",
    0x04:"DOP",
    0x61:"EOE",
    0x39:"GEOFENCE",
    0x09:"ODO",
    0x34:"ORB",
    0x01:"POSECEF",
    0x02:"POSLLH",
    0x07:"PVT",
    0x10:"RESETODO",
    0x35:"SAT",
    0x32:"SBAS",
    0x06:"SOL",
    0x03:"STATUS",
    0x30:"SVINFO",
    0x24:"TIMEBDS",
    0x25:"TIMEGAL",
    0x23:"TIMEGLO",
    0x20:"TIMEGPS",
    0x26:"TIMELS",
    0x21:"TIMEUTC",
    0x11:"VELECEF",
    0x12:"VELNED",
}

_rxm_dict = {
    "class":"RXM",
    0x61:"IMES",
    0x41:"PMREQ",
    0x15:"RAWX",
    0x59:"RLM",
    0x13:"SFRBX",
    0x20:"SVSI",
}

_sec_dict = {
    "class":"SEC",
    0x01:"SIGN",
    0x03:"UNIQID"
}

_upd_dict = {
    "class":"UPD",
    0x14:"SOS"
}

_tim_dict = {
    "class":"TIM",
    0x11:"DOSC",
    0x16:"FCHG",
    0x17:"HOC",
    0x13:"SMEAS",
    0x04:"SVIN",
    0x03:"TM2",
    0x12:"TOS",
    0x01:"TP",
    0x15:"VCOCAL",
    0x06:"VRFY",
}

_mga_dict = {
    "class":"MGA",
    0x60:"ACK-DATA0",
    0x20:"ANO",
    0x03:"BDS",
    0x80:"DBD",
    0x21:"FLASH",
    0x02:"GAL",
    0x06:"GLO",
    0x00:"GPS",
    0x40:"INI",
    0x05:"QZSS-EPH",
}

_class_dict = {
    0x01: _nav_dict,
    0x02: _rxm_dict,
    0x04: _inf_dict,
    0x05: _ack_dict,
    0x06: _cfg_dict,
    0x09: _upd_dict,
    0x0A: _mon_dict,
    0x0B: _aid_dict,
    0x0D: _tim_dict,
    0x10: _esf_dict,
    0x13: _mga_dict,
    0x21: _log_dict,
    0x27: _sec_dict,
}

def _key_lookup(d, value):
    for key in d:
        if type(key) != str: # Ignore a dictionary header e.g. class":"MGA"
            if d[key] == value:
                return key


def _class_lookup(cls):
    for key in _class_dict:
        if _class_dict[key]['class'] == cls:
            return key


def _checksum(class_and_payload):
    ck_a = 0
    ck_b = 0
    for byte in class_and_payload:
        ck_a = ck_a + byte
        ck_b = ck_b + ck_a
    ck = struct.pack('<BB', ck_a & 0xFF, ck_b & 0xFF)
    return ck


def ubx_extract(data):
    try:
        pos = data.index(struct.unpack("B", _Sync.SYNC1)[0])
    except:
        return (b'', b'')
    if pos < 0:
        return (b'', b'')
    if (len(data) - pos - _UBX_MIN_MESSAGE_LEN) < 0:
        return (b'', data[pos:])
    if data[pos+1] != struct.unpack("B", _Sync.SYNC2)[0]:
        return (b'', data[pos+2:])
    [length] = struct.unpack('<H', data[pos+4:pos+6])
    if (len(data) - pos - _UBX_MIN_MESSAGE_LEN) < length:
        return (b'', data[pos:])
    ck = _checksum(data[pos+2:pos+6+length])
    if ck[0] != data[pos+6+length] or \
        ck[1] != data[pos+7+length]:
        print('Failed')
        return (b'', data[pos:])
    else:
        return (data[pos:pos+_UBX_MIN_MESSAGE_LEN+length], data[pos+_UBX_MIN_MESSAGE_LEN+length:])


def ubx_build(cls, msg_id, payload):
    hdr = struct.pack('<BBH', cls, msg_id, len(payload))
    msg = _Sync.SYNC1 + _Sync.SYNC2 + hdr + payload
    return msg + _checksum(msg[2:])


def ubx_to_string(msg):
    (cls, msg_id) = struct.unpack('BB', msg[2:4])
    if cls in _class_dict:
        text = _class_dict[cls]['class']
        if msg_id in _class_dict[cls]:
            text = text + '-' + _class_dict[cls][msg_id]
        else:
            text = text + '-???(%02x)' % msg_id
    else:
        text = '???(%02x)-???(%02x)' % (cls, msg_id)
    return text


def ubx_string_to_cls_msg_id(s):
    (cls, msg_id) = s.split('-')
    cls = _class_lookup(cls)
    msg_id = _key_lookup(_class_dict[cls], msg_id)
    return (cls, msg_id)


def ubx_mga_flash_data(sequence, mga_ano_payload):
    (cls, msg_id) = ubx_string_to_cls_msg_id('MGA-FLASH')
    payload = struct.pack('<BBHH', 0x01, 0x00, sequence, len(mga_ano_payload))
    return ubx_build(cls, msg_id, payload + mga_ano_payload)


def ubx_mga_flash_stop():
    (cls, msg_id) = ubx_string_to_cls_msg_id('MGA-FLASH')
    payload = struct.pack('<BB', 0x02, 0x00)
    return ubx_build(cls, msg_id, payload)


def ubx_cfg_save_flash():
    (cls, msg_id) = ubx_string_to_cls_msg_id('CFG-CFG')
    payload = struct.pack('<IIIB', 0, 0xFFFFFFFF, 0, 0x3)
    return ubx_build(cls, msg_id, payload)


def ubx_cfg_uart(baudrate):
    (cls, msg_id) = ubx_string_to_cls_msg_id('CFG-PRT')
    payload = struct.pack('<BxH2I3H2x',
                          1, 0, (3 << 6) | (4 << 9),
                          baudrate,
                          1, 1, 0)
    return ubx_build(cls, msg_id, payload)


def ubx_mga_flash_ack_extract(msg):
    (_, _, ack, _, sequence) = struct.unpack('<BBBBH', msg[6:12])
    return (ack, sequence)




# Convert ASCII configuration message as retrieved using u-center
# to an actual UBX message which can be sent directly to the MN8
# E.g., CFG-MSG - 06 01 08 00 01 06 00 00 00 00 00 00
#                 |   |  |
#                 cls |  |             
#                     id |
#                        len
# This involves discarding the header "CFG-xxx" - header and then
# converting each ASCII byte to a real byte, prepending the SYNC
# bytes and then appending the CK_A and CK_B checksum bytes.

def ubx_build_from_ascii_cfg(text):
    msg = text.split(' ')
    if 'CFG' not in msg[0]: return ''
    data = _Sync.SYNC1 + _Sync.SYNC2 + b''.join([struct.pack('B', int(i, 16)) for i in msg[2:]])
    return data + _checksum(data[2:])


def ubx_build_from_ascii(text):
    data = _Sync.SYNC1 + _Sync.SYNC2 + binascii.unhexlify(text)
    return data + _checksum(data[2:])

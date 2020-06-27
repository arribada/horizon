/* M8N.h - GPS device Ublox Neo-M8N register descriptions
 *
 * Copyright (C) 2018 Arribada
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _M8N_H_
#define _M8N_H_

#include <stdint.h>

#define GPS_UART_NO_ERROR                   ( 0)
#define GPS_UART_ERROR_INSUFFICIENT_BYTES   (-1)
#define GPS_UART_ERROR_MISSING_SYNC1        (-2)
#define GPS_UART_ERROR_MISSING_SYNC2        (-3)
#define GPS_UART_ERROR_MSG_TOO_BIG          (-4)
#define GPS_UART_ERROR_MSG_PENDING          (-5)
#define GPS_UART_ERROR_CHECKSUM             (-6)
#define GPS_UART_ERROR_BACKEND              (-7)

#define UBX_PACKET_SYNC_CHAR1  (0xB5)
#define UBX_PACKET_SYNC_CHAR2  (0x62)

#define UBX_HEADER_AND_CRC_LENGTH  (8)
#define UBX_HEADER_LENGTH          (6)
#define UBX_CRC_LENGTH             (2)
#define UBX_MAX_PACKET_LENGTH      (1016)

#define UBX_SET_PACKET_HEADER(p, cls, id, length) \
    (p)->syncChars[0] = UBX_PACKET_SYNC_CHAR1; \
    (p)->syncChars[1] = UBX_PACKET_SYNC_CHAR2; \
    (p)->msgClass = cls; \
    (p)->msgId = id; \
    (p)->msgLength = length

#define UBX_IS_MSG(p, cls, id) (cls == (p)->msgClass && id == (p)->msgId)

#define UBX_PAYLOAD(p, t)      (&((p)->t))

#define UBX_PACKET_SIZE(p) UBX_HEADER_AND_CRC_LENGTH + (p)->msgLength

typedef enum
{
    UBX_PORT_DDC,
    UBX_PORT_UART,
    UBX_PORT_USB = 3,
    UBX_PORT_SPI
} UBX_PortID_t;

typedef enum
{
    UBX_MSG_CLASS_NAV = 0x01,
    UBX_MSG_CLASS_RXM = 0x02,
    UBX_MSG_CLASS_INF = 0x04,
    UBX_MSG_CLASS_ACK = 0x05,
    UBX_MSG_CLASS_CFG = 0x06,
    UBX_MSG_CLASS_MON = 0x0A,
    UBX_MSG_CLASS_AID = 0x0B,
    UBX_MSG_CLASS_TIM = 0x0D,
    UBX_MSG_CLASS_LOG = 0x21
} UBX_MessageClass_t;

typedef enum
{
    UBX_MSG_ID_MON_HW   = 0x09,
    UBX_MSG_ID_MON_HW2  = 0x0B,
    UBX_MSG_ID_MON_RXR  = 0x21,
} UBX_MessageID_MON_t;

typedef enum
{
    UBX_MSG_ID_INF_ERROR     = 0x00,
    UBX_MSG_ID_INF_WARNING   = 0x01,
    UBX_MSG_ID_INF_NOTICE    = 0x02,
    UBX_MSG_ID_INF_TEST      = 0x03,
    UBX_MSG_ID_INF_DEBUG     = 0x04
} UBX_MessageID_INF_t;

typedef enum
{
    UBX_MSG_ID_RXM_PMREQ     = 0x41,
} UBX_MessageID_RXM_t;

typedef enum
{
    UBX_MSG_ID_ACK_ACK       = 0x01,
    UBX_MSG_ID_ACK_NACK      = 0x00
} UBX_MessageID_ACK_t;

typedef enum
{
    UBX_MSG_ID_NAV_AOPSTATUS = 0x60,
    UBX_MSG_ID_NAV_CLOCK     = 0x22,
    UBX_MSG_ID_NAV_DGPS      = 0x31,
    UBX_MSG_ID_NAV_DOP       = 0x04,
    UBX_MSG_ID_NAV_POSECEF   = 0x01,
    UBX_MSG_ID_NAV_POSLLH    = 0x02,
    UBX_MSG_ID_NAV_PVT       = 0x07,
    UBX_MSG_ID_NAV_SBAS      = 0x32,
    UBX_MSG_ID_NAV_SOL       = 0x06,
    UBX_MSG_ID_NAV_STATUS    = 0x03,
    UBX_MSG_ID_NAV_SVINFO    = 0x30,
    UBX_MSG_ID_NAV_TIMEGPS   = 0x20,
    UBX_MSG_ID_NAV_TIMEUTC   = 0x21,
    UBX_MSG_ID_NAV_VELECEF   = 0x11,
    UBX_MSG_ID_NAV_VELNED    = 0x12
} UBX_MessageID_NAV_t;

typedef enum
{
    UBX_MSG_ID_LOG_CREATE = 0x07,
    UBX_MSG_ID_LOG_ERASE = 0x03,
    UBX_MSG_ID_LOG_STRING = 0x04,
    UBX_MSG_ID_LOG_INFO = 0x08,
} UBX_MessageID_LOG_t;

/****************************** ACK *************************************/

/* This message can be concatenated multiple times into a single message */
typedef struct __attribute__((__packed__))
{
    uint8_t  clsID;
    uint8_t  msgID;
} UBX_ACK_t;

typedef struct __attribute__((__packed__))
{
    uint8_t  clsID;
    uint8_t  msgID;
} UBX_NACK_t;

/****************************** CFG *************************************/

typedef enum
{
    UBX_MSG_ID_CFG_PRT       = 0x00,
    UBX_MSG_ID_CFG_ANT       = 0x13,
    UBX_MSG_ID_CFG_CFG       = 0x09,
    UBX_MSG_ID_CFG_DAT       = 0x06,
    UBX_MSG_ID_CFG_GNSS      = 0x3E,
    UBX_MSG_ID_CFG_INF       = 0x02,
    UBX_MSG_ID_CFG_ITFM      = 0x39,
    UBX_MSG_ID_CFG_LOGFILTER = 0x47,
    UBX_MSG_ID_CFG_MSG       = 0x01,
    UBX_MSG_ID_CFG_NAV5      = 0x24,
    UBX_MSG_ID_CFG_NAVX5     = 0x23,
    UBX_MSG_ID_CFG_NMEA      = 0x17,
    UBX_MSG_ID_CFG_NVS       = 0x22,
    UBX_MSG_ID_CFG_PM2       = 0x38,
    UBX_MSG_ID_CFG_RATE      = 0x08,
    UBX_MSG_ID_CFG_RINV      = 0x34,
    UBX_MSG_ID_CFG_RST       = 0x04,
    UBX_MSG_ID_CFG_RXM       = 0x11,
    UBX_MSG_ID_CFG_SBAS      = 0x16,
    UBX_MSG_ID_CFG_TP5       = 0x31,
    UBX_MSG_ID_CFG_USB       = 0x1B
} UBX_MessageID_CFG_t;

#define UBX_CFG_CFG_MASK_IOPORT      0x0001U
#define UBX_CFG_CFG_MASK_MSGCONF     0x0002U
#define UBX_CFG_CFG_MASK_INFMSG      0x0004U
#define UBX_CFG_CFG_MASK_NAVCONF     0x0008U
#define UBX_CFG_CFG_MASK_RXMCONF     0x0010U
#define UBX_CFG_CFG_MASK_RINVCONF    0x0200U
#define UBX_CFG_CFG_MASK_ANTCONF     0x0400U
#define UBX_CFG_CFG_MASK_LOGCONF     0x0800U
#define UBX_CFG_CFG_MASK_FTSCONF     0x1000U

#define UBX_CFG_CFG_DEV_BBR          0x01U
#define UBX_CFG_CFG_DEV_FLASH        0x02U
#define UBX_CFG_CFG_DEV_EEPROM       0x04U
#define UBX_CFG_CFG_DEV_SPIFLASH     0x10U

typedef struct __attribute__((__packed__))
{
    uint32_t clearMask;
    uint32_t saveMask;
    uint32_t loadMask;
} UBX_CFG_CFG_t;

typedef struct __attribute__((__packed__))
{
    uint32_t clearMask;
    uint32_t saveMask;
    uint32_t loadMask;
    uint8_t  deviceMask;
} UBX_CFG_CFG2_t;

#define UBX_CFG_RST_NAV_BBR_MASK_EPH    0x0001
#define UBX_CFG_RST_NAV_BBR_MASK_ALM    0x0002
#define UBX_CFG_RST_NAV_BBR_MASK_HEALTH 0x0004
#define UBX_CFG_RST_NAV_BBR_MASK_KLOB   0x0008
#define UBX_CFG_RST_NAV_BBR_MASK_POS    0x0010
#define UBX_CFG_RST_NAV_BBR_MASK_CLKD   0x0020
#define UBX_CFG_RST_NAV_BBR_MASK_OSC    0x0040
#define UBX_CFG_RST_NAV_BBR_MASK_UTC    0x0080
#define UBX_CFG_RST_NAV_BBR_MASK_RTC    0x0100
#define UBX_CFG_RST_NAV_BBR_MASK_AOP    0x8000

typedef enum
{
    UBX_CFG_RST_MODE_HWRESET_NOW  = 0x00,
    UBX_CFG_RST_MODE_SWRESET      = 0x01,
    UBX_CFG_RST_MODE_SWRESET_GNSS = 0x02,
    UBX_CFG_RST_MODE_HWRESET_SHUT = 0x04,
    UBX_CFG_RST_MODE_GNSS_STOP    = 0x08,
    UBX_CFG_RST_MODE_GNSS_START   = 0x09
} UBX_CFG_RST_MODE_t;

typedef struct __attribute__((__packed__))
{
    uint16_t  navBbrMask;
    uint8_t   resetMode;
    uint8_t   reserved1;
} UBX_CFG_RST_t;

#define UBX_RXM_PMREQ_FLAGS_BACKUP   0x00000002U
#define UBX_RXM_PMREQ_FLAGS_FORCE    0x00000004U


typedef struct __attribute__((__packed__))
{
    uint32_t duration;
    uint32_t flags;
} UBX_RXM_PMREQ_t;

#define UBX_RXM_PMREQ_VERSION        0
#define UBX_RXM_PMREQ_WAKEUP_UARTRX  0x00000008U // Wake on UART RX pin
#define UBX_RXM_PMREQ_WAKEUP_EXTINT0 0x00000020U
#define UBX_RXM_PMREQ_WAKEUP_EXTINT1 0x00000040U
#define UBX_RXM_PMREQ_WAKEUP_SPICS   0x00000080U

typedef struct __attribute__((__packed__))
{
    uint8_t  version;
    uint8_t  reserved[3];
    uint32_t duration;
    uint32_t flags;
    uint32_t wakeupSources;
} UBX_RXM_PMREQ2_t;

#define UBX_CFG_LOGFILTER_VERSION        1U

#define UBX_CFG_LOGFILTER_FLAGS_REC      1U
#define UBX_CFG_LOGFILTER_FLAGS_PSMONCE  2U
#define UBX_CFG_LOGFILTER_FLAGS_APPLYALL 4U

typedef struct __attribute__((__packed__))
{
    uint8_t  version;           /* Set to 1 */
    uint8_t  flags;
    uint16_t minInterval;       /* 0 => not set */
    uint16_t timeThreshold;     /* 0 => not set */
    uint16_t speedThreshold;    /* 0 => not set */
    uint32_t positionThreshold; /* 0 => not set */
} UBX_CFG_LOGFILTER_t;

#define UBX_CFG_INPROTO_MASK_UBX    0x01
#define UBX_CFG_INPROTO_MASK_NMEA   0x02
#define UBX_CFG_INPROTO_MASK_RTCM   0x04

#define UBX_CFG_OUTPROTO_MASK_UBX   0x01
#define UBX_CFG_OUTPROTO_MASK_NMEA  0x02

typedef struct __attribute__((__packed__))
{
    uint8_t  portID;
} UBX_CFG_PRT1_t;

typedef struct __attribute__((__packed__))
{
    uint8_t  portID;
    uint8_t  reserved0[1];
    uint16_t txReady;
    uint32_t mode;
    uint32_t baudRate;
    uint16_t inProtoMask;
    uint16_t outProtoMask;
    uint16_t flags;
    uint8_t  reserved5[2];
} UBX_CFG_PRT2_t;

typedef struct __attribute__((__packed__))
{
    uint8_t  msgClass;
    uint8_t  msgID;
} UBX_CFG_MSG_POLL_t;

typedef struct __attribute__((__packed__))
{
    uint8_t  msgClass;
    uint8_t  msgID;
    uint8_t  rate;
} UBX_CFG_MSG_t;

typedef struct __attribute__((__packed__))
{
    uint8_t   protocolID;
    uint8_t   reserved0;
    uint16_t  reserved1;
    uint8_t   infMsgMask[6];
} UBX_CFG_INFO_t;

typedef struct __attribute__((__packed__))
{
    uint16_t  flags;
    uint16_t  pins;
} UBX_CFG_ANT_t;

typedef struct __attribute__((__packed__))
{
    uint16_t mask;
    uint8_t  dynModel;
    uint8_t  fixMode;
    int32_t  fixedAlt;
    uint32_t fixedAltVar;
    int8_t   minElev;
    uint8_t  drLimit;
    uint16_t pDop;
    uint16_t tDop;
    uint16_t pAcc;
    uint16_t tAcc;
    uint8_t  staticHoldThresh;
    uint8_t  dgpsTimeOut;
    uint8_t  cnoThreshNumSVs;
    uint8_t  cnoThresh;
    uint16_t reserved2;
    uint32_t reserved3;
    uint32_t reserved4;
} UBX_CFG_NAV5_t;

/****************************** LOG *************************************/

#define UBX_LOG_CREATE_VERSION         0U
#define UBX_LOG_CREATE_LOGCFG_CIRCULAR 1U

#define UBX_LOG_CREATE_LOGSIZE_MAX     0U
#define UBX_LOG_CREATE_LOGSIZE_MIN     1U
#define UBX_LOG_CREATE_LOGSIZE_USER    2U

typedef struct __attribute__((__packed__))
{
    uint8_t version;            /* Set to 0 */
    uint8_t logCfg;
    uint8_t reserved1[1];
    uint8_t logSize;
    uint32_t userDefinedSize;
} UBX_LOG_CREATE_t;

#define UBX_LOG_STRING_MAX_LENGTH      256

typedef struct __attribute__((__packed__))
{
    uint8_t bytes[UBX_LOG_STRING_MAX_LENGTH];
} UBX_LOG_STRING_t;

#define UBX_LOG_INFO_STATUS_RECORDING  0x08U
#define UBX_LOG_INFO_STATUS_INACTIVE   0x10U
#define UBX_LOG_INFO_STATUS_CIRCULAR   0x20U

typedef struct __attribute__((__packed__))
{
    uint8_t  version;
    uint8_t  reserved1[3];
    uint32_t filestoreCapacity;
    uint8_t  reserved2[8];
    uint32_t currentMaxLogSize;
    uint32_t currentLogSize;
    uint32_t entryCount;
    uint16_t oldestYear;
    uint8_t  oldestMonth;
    uint8_t  oldestDay;
    uint8_t  oldestHour;
    uint8_t  oldestMinute;
    uint8_t  oldestSecond;
    uint8_t  reserved3[1];
    uint16_t newestYear;
    uint8_t  newestMonth;
    uint8_t  newestDay;
    uint8_t  newestHour;
    uint8_t  newestMinute;
    uint8_t  newestSecond;
    uint8_t  reserved4[1];
    uint8_t  status;
    uint8_t  reserved5[3];
} UBX_LOG_INFO_t;

/****************************** NAV *************************************/

#define UBX_NAV_STATUS_FLAGS_GPSFIXOK  0x01U
#define UBX_NAV_STATUS_FLAGS_DIFFSOLN  0x02U
#define UBX_NAV_STATUS_FLAGS_WKNSET    0x04U
#define UBX_NAV_STATUS_FLAGS_TOWSET    0x08U

#define UBX_NAV_STATUS_FIXSTAT_DGPSISTAT   0x01U
#define UBX_NAV_STATUS_FIXSTAT_MAPMATCHING 0xC0U

enum {
    UBX_NAV_STATUS_FIXSTAT_MAPMATCHING_NONE,
    UBX_NAV_STATUS_FIXSTAT_MAPMATCHING_VALID_NOT_USED,
    UBX_NAV_STATUS_FIXSTAT_MAPMATCHING_VALID_AND_USED,
    UBX_NAV_STATUS_FIXSTAT_MAPMATCHING_VALID_AND_USED_DR,
};

#define UBX_NAV_STATUS_FLAGS2_PSMSTATE      0x03U

enum {
    UBX_NAV_STATUS_FLAGS2_PSMSTATE_ACQUISITION,
    UBX_NAV_STATUS_FLAGS2_PSMSTATE_TRACKING,
    UBX_NAV_STATUS_FLAGS2_PSMSTATE_OPTIMIZED,
    UBX_NAV_STATUS_FLAGS2_PSMSTATE_INACTIVE,
};

#define UBX_NAV_STATUS_FLAGS2_SPOOFDETSTATE 0x18U

enum {
    UBX_NAV_STATUS_FLAGS2_SPOOFDETSTATE_UNKNOWN_OFF,
    UBX_NAV_STATUS_FLAGS2_SPOOFDETSTATE_INDICATED,
    UBX_NAV_STATUS_FLAGS2_SPOOFDETSTATE_MULTI_INDICATED,
};

enum {
    UBX_NAV_STATUS_GPSFIX_NOFIX,
    UBX_NAV_STATUS_GPSFIX_DEAD_RECKONING,
    UBX_NAV_STATUS_GPSFIX_2DFIX,
    UBX_NAV_STATUS_GPSFIX_3DFIX,
    UBX_NAV_STATUS_GPSFIX_GPS_DEAD_RECKONING,
    UBX_NAV_STATUS_GPSFIX_TIME_ONLY
};

typedef struct __attribute__((__packed__))
{
    uint32_t iTOW;
    uint8_t  gpsFix;
    uint8_t  flags;
    uint8_t  fixStat;
    uint8_t  flags2;
    uint32_t ttff;
    uint32_t msss;
} UBX_NAV_STATUS_t;

typedef struct __attribute__((__packed__))
{
    uint32_t iTOW;   // GPS time of week of the navigation epoch
    int32_t  lon;    // Longitude
    int32_t  lat;    // Latitude
    int32_t  height; // Height above ellipsoid
    int32_t  hMSL;   // Height above mean sea level
    uint32_t hAcc;   // Horizontal accuracy estimate
    uint32_t vAcc;   // Vertical accuracy estimate
} UBX_NAV_POSLLH_t;

enum UBX_NAV_PVT_FIXTYPE {
    UBX_NAV_PVT_FIXTYPE_NO_FIX = 0,
    UBX_NAV_PVT_FIXTYPE_FIX_2D = 2,
    UBX_NAV_PVT_FIXTYPE_FIX_3D = 3
};

#define UBX_NAV_PVT_VALID_FLAGS_DATE (1 << 0)
#define UBX_NAV_PVT_VALID_FLAGS_TIME (1 << 1)

typedef struct __attribute__((__packed__))
{
    uint32_t iTOW;         // GPS time of week of the navigation epoch
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
    uint8_t  valid;        // Validity flags
    uint32_t tAcc;         // Time accuracy estimate in nanoseconds
    int32_t  nano;
    uint8_t  fixType;      // GNSSfix Type
    uint8_t  flags;        // Fix status flags
    uint8_t  flags2;       // Additional flags
    uint8_t  numSV;        // Number of satellites used in Nav Solution
    int32_t  lon;          // Longitude
    int32_t  lat;          // Latitude
    int32_t  height;       // Height above ellipsoid
    int32_t  hMSL;         // Height above mean sea level
    uint32_t hAcc;         // Horizontal accuracy estimate
    uint32_t vAcc;         // Vertical accuracy estimate
    int32_t  velN;         // NED north velocity
    int32_t  velE;         // NED east velocity
    int32_t  velD;         // NED down velocity
    int32_t  gSpeed;       // Ground speed (2D)
    int32_t  headMot;      // Heading of motion (2D)
    uint32_t sAcc;         // Speed accuracy estimate
    uint32_t headAcc;      // Heading accuracy estimate (both motion and  vehicle)
    uint16_t pDOP;         // Position DOP
    uint8_t  reserved1[6];
    int32_t  headVeh;      // Heading of vehicle (2D)
    int16_t  magDec;       // Magnetic declination
    uint16_t magAcc;       // Magnetic declination accuracy
} UBX_NAV_PVT_t;

/****************************** MON *************************************/

typedef struct __attribute__((__packed__))
{
    uint8_t  flags;
} UBX_MON_RXR_t;

typedef struct __attribute__((__packed__))
{
    int8_t   ofsI;
    uint8_t  magI;
    int8_t   ofsQ;
    uint8_t  magQ;
    uint8_t  cfgSource;
    uint8_t  reserved0[3];
    uint32_t lowLevCfg;
    uint32_t reserved1[4];
    uint32_t postStatus;
    uint32_t reserved2;
} UBX_MON_HW2_t;

typedef struct __attribute__((__packed__))
{
    uint8_t  syncChars[2];
    uint8_t  msgClass;
    uint8_t  msgId;
    uint16_t msgLength;            /* Excludes header and CRC bytes */
    /* Payload must be 32-bit aligned */
    union
    {
        uint8_t             payloadAndCrc[256];  /* CRC is appended to payload */
        UBX_ACK_t           UBX_ACK;
        UBX_NACK_t          UBX_NACK;
        UBX_CFG_CFG_t       UBX_CFG_CFG;
        UBX_CFG_CFG2_t      UBX_CFG_CFG2;
        UBX_CFG_MSG_POLL_t  UBX_CFG_MSG_POLL;
        UBX_CFG_PRT2_t      UBX_CFG_PRT2;
        UBX_LOG_INFO_t      UBX_LOG_INFO;
        UBX_LOG_CREATE_t    UBX_LOG_CREATE;
        UBX_RXM_PMREQ2_t    UBX_RXM_PMREQ2;
        UBX_NAV_STATUS_t    UBX_NAV_STATUS;
        UBX_NAV_POSLLH_t    UBX_NAV_POSLLH;
        UBX_CFG_LOGFILTER_t UBX_CFG_LOGFILTER;
        UBX_NAV_PVT_t       UBX_NAV_PVT;
    };
} UBX_Packet_t;

/****************************** Functions ********************************/

void UBX_SetChecksum(UBX_Packet_t *packet);
int UBX_CheckChecksum(UBX_Packet_t *packet);

#endif /* _M8N_H_ */

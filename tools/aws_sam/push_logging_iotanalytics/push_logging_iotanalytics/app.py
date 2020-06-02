import logging
import base64
import boto3
from . import log
import json
import uuid
import datetime
import time

logger = logging.getLogger()
logger.setLevel(logging.INFO)

def convert_date_time_to_timestamp(t):
    dt = datetime.datetime(t.year, t.month, t.day, t.hours, t.minutes, t.seconds)
    return int(time.mktime(dt.timetuple()))


def send_batch_messages(client, channel_name, messages):
    # Send messages in chunks to avoid hitting AWS service limits
    chunk_size = 100
    for chunk in range(0, len(messages), chunk_size):
        resp = client.batch_put_message(channelName=channel_name, messages=messages[chunk:chunk+chunk_size])
        logger.debug('resp=%s', resp)


def lambda_handler(event, context):

    thing_name = event['thing_name']
    log_data = base64.b64decode(event['data'])

    logger.info('raw_data: %s,%s,%s', thing_name, len(log_data), event['data'])
    logger.debug('Got binary data length of %s bytes from thing %s', len(log_data), event['thing_name'])

    log_entries = log.decode_all(log_data)
    logger.debug('Decoded %s log entries', len(log_entries))

    iot_client = boto3.client('iotanalytics')

    # Keep track of log entries of interest
    gps_pos = None
    ttff = None
    timestamp = None
    battery_charge = None
    battery_voltage = None

    iot_gps_messages = []
    iot_battery_messages = []

    for i in log_entries:

        # Check for log entries of interest
        if i.tag == log.LogItem_GPS_TimeToFirstFix.tag:
            ttff = i
        elif i.tag == log.LogItem_GPS_Position.tag:
            gps_pos = i
        elif i.tag == log.LogItem_Time_DateTime.tag:
            timestamp = convert_date_time_to_timestamp(i)
        elif i.tag == log.LogItem_Time_Timestamp.tag:
            timestamp = i.timestamp
        elif i.tag == log.LogItem_Battery_Charge.tag:
            battery_charge = i
        elif i.tag == log.LogItem_Battery_Voltage.tag:
            battery_voltage = i

        if gps_pos is not None and timestamp is not None:
            uid = uuid.uuid4()
            entry = { 'thing_name': thing_name,
                      'timestamp': timestamp,
                      'longitude': gps_pos.longitude,
                      'latitude': gps_pos.latitude,
                      'height': gps_pos.height,
                      'h_acc': gps_pos.accuracyHorizontal,
                      'v_acc': gps_pos.accuracyVertical
            }
            if ttff:
                entry['ttff'] = ttff.ttff / 1000.0

            #logger.debug('GPS Location: %s', entry)
            iot_gps_messages.append({
                'messageId': str(uid),
                'payload': json.dumps(entry)
                })

            # Reset fields
            gps_pos = None
            ttff = None
            timestamp = None

        if battery_charge is not None and timestamp is not None:
            # Create table entry            
            uid = uuid.uuid4()
            entry = { 'thing_name': thing_name,
                      'timestamp': timestamp,
                      'battery_level': battery_charge.charge
            }
            #logger.debug('Battery: %s', entry)

            iot_battery_messages.append({
                'messageId': str(uid),
                'payload': json.dumps(entry)
                })

            # Reset fields
            battery_charge = None      
            timestamp = None

        if battery_voltage is not None and timestamp is not None:
            # Create table entry            
            uid = uuid.uuid4()
            entry = { 'thing_name': thing_name,
                      'timestamp': timestamp,
                      'battery_voltage': battery_voltage.voltage
            }
            #logger.debug('Battery: %s', entry)

            iot_battery_messages.append({
                'messageId': str(uid),
                'payload': json.dumps(entry)
                })

            # Reset fields
            battery_voltage = None      
            timestamp = None

    if iot_gps_messages:
        send_batch_messages(iot_client, 'arribada_gps_location', iot_gps_messages)

    if iot_battery_messages:
        send_batch_messages(iot_client, 'arribada_battery_charge', iot_battery_messages)

    return {
        "statusCode": 200
    }

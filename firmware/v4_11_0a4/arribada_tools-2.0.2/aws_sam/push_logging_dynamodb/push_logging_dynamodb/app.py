import logging
import base64
import boto3
from . import log
import uuid
import datetime
import time

logger = logging.getLogger()
logger.setLevel(logging.INFO)

def convert_date_time_to_timestamp(t):
    dt = datetime.datetime(t.year, t.month, t.day, t.hours, t.minutes, t.seconds)
    return int(time.mktime(dt.timetuple()))


def lambda_handler(event, context):

    thing_name = event['thing_name']
    log_data = base64.b64decode(event['data'])
    logger.debug('Got binary data length %s from %s', len(log_data), event['thing_name'])

    log_entries = log.decode_all(log_data)
    logger.debug('Got %s log entries', len(log_entries))

    db_client = boto3.client('dynamodb')

    # Keep track of log entries of interest
    gps_pos = None
    ttff = None
    timestamp = None
    battery = None

    for i in log_entries:

        # Check for log entries of interest
        if i.tag == log.LogItem_GPS_TimeToFirstFix.tag:
            ttff = i
        elif i.tag == log.LogItem_GPS_Position.tag:
            gps_pos = i
        elif i.tag == log.LogItem_Time_DateTime.tag:
            timestamp = convert_date_time_to_timestamp(i)
        elif i.tag == log.LogItem_Battery_Charge.tag:
            battery = i

        if gps_pos is not None and timestamp is not None:
            uid = uuid.uuid4()
            entry = { 'uuid': { 'S': str(uid) },
                      'thing_name': { 'S': thing_name },
                      'timestamp': { 'N': str(timestamp) },
                      'longitude': { 'N': str(gps_pos.longitude) },                      
                      'latitude': { 'N': str(gps_pos.latitude) },
                      'height': { 'N': str(gps_pos.height) },
                      'h_acc': { 'N': str(gps_pos.accuracyHorizontal) },
                      'v_acc': { 'N': str(gps_pos.accuracyVertical) }
            }
            if ttff:
                entry['ttff'] = { 'N': str((ttff.ttff / 1000.0)) }
            #logger.debug('GPS Location: %s', entry)

            resp = db_client.put_item(TableName='ArribadaGPSLocation', Item=entry)
            logger.debug('resp=%s', resp)

            # Reset fields
            gps_pos = None
            ttff = None
            timestamp = None

        if battery is not None and timestamp is not None:
            # Create table entry            
            uid = uuid.uuid4()
            entry = { 'uuid': { 'S': str(uid) },
                      'thing_name': { 'S': thing_name },
                      'timestmap': { 'N': str(timestamp) },
                      'battery_level': { 'N': str(battery.charge) }
            }
            #logger.debug('Battery: %s', entry)

            resp = db_client.put_item(TableName='ArribadaBatteryCharge', Item=entry)
            logger.debug('resp=%s', resp)

            # Reset fields
            battery = None      
            timestamp = None

    return {
        "statusCode": 200
    }

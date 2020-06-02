import logging
import boto3
import uuid

logger = logging.getLogger()
logger.setLevel(logging.ERROR)

def lambda_handler(event, context):

    thing_name = event['thing_name']
    timestamp = event['update']['timestamp']
    current_record = event['update']['current']['state']['desired']

    if 'device_status' not in current_record:
        logger.error('Shadow has no "device_status" field')
        return { 'statusCode': 200 }
    else:
        device_status = current_record['device_status']

    logger.info('Thing: %s State: %s Time: %s', thing_name, current_record, timestamp)

    db_client = boto3.client('dynamodb')
    uid = uuid.uuid4()
    entry = {}
    entry['uuid'] = { 'S': str(uid) }
    entry['thing_name'] = { 'S': thing_name }
    entry['timestamp'] = { 'N': str(timestamp) }
    if 'last_gps_location' in device_status:
        gps = device_status['last_gps_location']
        entry['last_gps_fix_longitude'] = { 'N': str(gps['longitude']) }
        entry['last_gps_fix_latitude'] = { 'N': str(gps['latitude']) }
        entry['last_gps_fix_time'] = { 'N': str(gps['timestamp']) }
    if 'last_cellular_connected_timestamp' in device_status:
        ts = device_status['last_cellular_connected_timestamp']
        entry['last_cellular_connection'] = { 'N': str(ts) }
    if 'last_sat_tx_timestamp' in device_status:
        ts = device_status['last_sat_tx_timestamp']
        entry['last_sat_tx'] = { 'N': str(ts) }
    if 'next_sat_tx_timestamp' in device_status:
        ts = device_status['next_sat_tx_timestamp']
        entry['next_sat_tx'] = { 'N': str(ts) }
    if 'battery_level' in device_status:
        n = device_status['battery_level']
        entry['battery_level'] = { 'N': str(n) }
    if 'configuration_version' in device_status:
        n = device_status['configuration_version']
        entry['config_version'] = { 'N': str(n) }
    if 'firmware_version' in device_status:
        n = device_status['firmware_version']
        entry['fw_version'] = { 'N': str(n) }
    if 'last_log_file_read_pos' in device_status:
        n = device_status['last_log_file_read_pos']
        entry['last_log_read_pos'] = { 'N': str(n) }

    resp = db_client.put_item(TableName='ArribadaDeviceStatus', Item=entry)
    logger.debug('resp=%s', resp)

    return { 'statusCode': 200 }

from . import config
from . import message
import logging
import binascii


logger = logging.getLogger(__name__)


class ExceptionFeatureDeprecated(Exception):
    pass


class ExceptionBackendCommsError(Exception):
    pass


def resp_error_handler(req, resp, expected_resp=None):
    if not resp:
        logger.error('%s got no response - expected %s', req, expected_resp)
        raise ExceptionBackendCommsError('TIMEOUT_ERROR')
    elif expected_resp and expected_resp != resp.name:
        logger.error('%s got unexpected response %s - expected %s', req, resp.name, expected_resp)
        raise ExceptionBackendCommsError('UNEXPECTED_RESPONSE_ERROR')
    elif resp.error_code:
        logger.error('%s got response %s with unexpected error code: %s', req, resp.name, message.str_error(resp.error_code))
        raise ExceptionBackendCommsError(message.str_error(resp.error_code))


class ConfigInterface(object):

    timeout = 2.0

    def __init__(self, backend):
        self._backend = backend

    def _erase_config(self):
        cmd = message.ConfigMessage_CFG_ERASE_REQ(cfg_tag=0xFFFF)
        resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'GENERIC_RESP')
    
    def _save_config(self):
        cmd = message.ConfigMessage_CFG_SAVE_REQ()
        resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'GENERIC_RESP')

    def gps_config(self, enable):
        cmd = message.ConfigMessage_GPS_CONFIG_REQ(enable=enable)
        resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'GENERIC_RESP')

    def cellular_config(self, enable):
        cmd = message.ConfigMessage_CELLULAR_CONFIG_REQ(enable=enable)
        resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'GENERIC_RESP')

    def write_json_configuration(self, json, erase=True):
        if erase:
            self._erase_config()
        objs = config.json_loads(json)
        config_data = config.encode_all(objs)
        cmd = message.ConfigMessage_CFG_WRITE_REQ(length=len(config_data))
        resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'GENERIC_RESP')
        length = self._backend.write(config_data, self.timeout)
        if length != len(config_data):
            logger.error('Failed to send all configuration bytes (%u/%u)', length, len(config_data))
            raise ExceptionBackendCommsError
        resp = self._backend.command_response(None, self.timeout)
        resp_error_handler(cmd.name, resp, 'CFG_WRITE_CNF')
        self._save_config()

    def read_json_configuration(self, tag=0xFFFF):
        cmd = message.ConfigMessage_CFG_READ_REQ(cfg_tag=tag)
        resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'CFG_READ_RESP')
        config_data = self._backend.read(resp.length, self.timeout)
        if resp.length != len(config_data) or resp.error_code:
            logger.error('Failed to receive expected configuration bytes (%u/%u)',
                         len(config_data), resp.length)
            raise ExceptionBackendCommsError
        objs = config.decode_all(config_data)
        return config.json_dumps(objs)

    def erase_configuration(self):
        self._erase_config()
        self._save_config()

    def create_log_file(self, file_type, sync_enable=False, max_size=0):
        cmd = message.ConfigMessage_LOG_CREATE_REQ(mode=file_type,
                                                   sync_enable=sync_enable,
                                                   max_file_size=max_size)
        resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'GENERIC_RESP')

    def erase_log_file(self):
        cmd = message.ConfigMessage_LOG_ERASE_REQ()
        # Could take 30 seconds to erase a large log file so use a
        # larger timeout period for this command
        resp = self._backend.command_response(cmd, 30 + self.timeout)
        resp_error_handler(cmd.name, resp, 'GENERIC_RESP')

    def read_log_file(self, start_offset=0, length=0):
        cmd = message.ConfigMessage_LOG_READ_REQ(start_offset=start_offset, length=length)
        resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'LOG_READ_RESP')
        return self._backend.read(resp.length, self.timeout)

    def fw_upgrade(self, image_type, data):
        crc = binascii.crc32(data) & 0xFFFFFFFF # Ensure CRC32 is unsigned
        cmd = message.ConfigMessage_FW_SEND_IMAGE_REQ(image_type=image_type,
                                                      image_length=len(data),
                                                      crc=crc)
        resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'GENERIC_RESP')
        length = self._backend.write(data, self.timeout)
        if length != len(data):
            logger.error('Failed to send all firmware data bytes (%u/%u)', length, len(data))
            raise ExceptionBackendCommsError
        resp = self._backend.command_response(None, 60.0)
        resp_error_handler(cmd.name, resp, 'FW_SEND_IMAGE_COMPLETE_CNF')
        if image_type != 'ARTIC':
            cmd = message.ConfigMessage_FW_APPLY_IMAGE_REQ(image_type=image_type)
            resp = self._backend.command_response(cmd, 30.0) # We require a much longer timeout here to allow time for the image to be applied
            resp_error_handler(cmd.name, resp, 'GENERIC_RESP')

    def reset(self, reset_type):
        cmd = message.ConfigMessage_RESET_REQ(reset_type=reset_type)
        if (reset_type == 'FLASH'):
            # Erase RAM copy of configuration before full FLASH reset
            self._erase_config()
            resp = self._backend.command_response(cmd, 40) # The FLASH erase takes ~30 seconds
        else:
            resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'GENERIC_RESP')

    def get_battery_status(self):
        cmd = message.ConfigMessage_BATTERY_STATUS_REQ()
        resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'BATTERY_STATUS_RESP')
        return message.convert_to_dict(resp)

    def get_status(self):
        cmd = message.ConfigMessage_STATUS_REQ()
        resp = self._backend.command_response(cmd, 10) # Status request can take up to 10 seconds as the cellular device boots
        resp_error_handler(cmd.name, resp, 'STATUS_RESP')
        return message.convert_to_dict(resp)

    def get_version(self):
        cmd = message.ConfigMessage_VERSION_REQ()
        resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'VERSION_RESP')
        if resp.cfg_version != config.__version__:
            logger.warn('Board firmware configuration version is V%u but tool expects V%u', resp.cfg_version, config.__version__)
        return message.convert_to_dict(resp)

    def test_mode(self, test_mode):
        cmd = message.ConfigMessage_TEST_REQ(test_mode=test_mode)
        resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'GENERIC_RESP')

    def read_flash_file(self):
        cmd = message.ConfigMessage_FLASH_DOWNLOAD_REQ()
        resp = self._backend.command_response(cmd, self.timeout)
        resp_error_handler(cmd.name, resp, 'FLASH_DOWNLOAD_RESP')
        logger.debug('Downloading flash file of size: ', resp.length)
        return self._backend.read(resp.length, self.timeout)

/**
  ******************************************************************************
  * @file     syshal_ble.c
  * @brief    System hardware abstraction layer for BLE
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2019 Arribada</center></h2>
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
  *
  ******************************************************************************
  */

#include <string.h> // memset()
#include "amt.h"
#include "syshal_ble.h"
#include "sys_config.h"
#include "app_error.h"
#include "sdk_errors.h"
#include "ble_advdata.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "ble_db_discovery.h"
#include "debug.h"
#include "iot.h"
#include "buffer.h"

#define APP_BLE_CONN_CFG_TAG   1  /**< A tag that refers to the BLE stack configuration. */
#define APP_BLE_OBSERVER_PRIO  3  /**< Application's BLE observer priority. You shouldn't need to modify this value. */

#define CONN_INTERVAL_DEFAULT           (uint16_t)(MSEC_TO_UNITS(7.5, UNIT_1_25_MS))    /**< Default connection interval used at connection establishment by central side. */
#define CONN_INTERVAL_MIN               (uint16_t)(MSEC_TO_UNITS(7.5, UNIT_1_25_MS))    /**< Minimum acceptable connection interval, in 1.25 ms units. */
#define CONN_INTERVAL_MAX               (uint16_t)(MSEC_TO_UNITS(500, UNIT_1_25_MS))    /**< Maximum acceptable connection interval, in 1.25 ms units. */
#define ADV_INTERVAL_DEFAULT            (uint16_t)(MSEC_TO_UNITS(20.0,   UNIT_0_625_MS)) /**< Default connection interval used at connection establishment by central side. */
#define ADV_INTERVAL_MIN                (uint16_t)(MSEC_TO_UNITS(20.0,   UNIT_0_625_MS)) /**< Minimum acceptable advertising interval, in 0.625 ms units. */
#define ADV_INTERVAL_MAX                (uint16_t)(MSEC_TO_UNITS(4000.0, UNIT_0_625_MS)) /**< Maximum acceptable advertising interval, in 0.625 ms units. */
#define CONN_SUP_TIMEOUT                (uint16_t)(MSEC_TO_UNITS(4000,  UNIT_10_MS))    /**< Connection supervisory timeout (4 seconds). */
#define SLAVE_LATENCY                   0                                               /**< Slave latency. */
#define DEVICE_NAME "Arribada_Tracker"

#define SYSHAL_BLE_RX_BUFFER_DEPTH 8

static nrf_ble_amts_t     m_amts;
static syshal_ble_init_t config;

NRF_BLE_GATT_DEF(m_gatt);                       /**< GATT module instance. */
NRF_BLE_QWR_DEF(m_qwr);                         /**< Context for the Queued Write module.*/
BLE_DB_DISCOVERY_DEF(m_ble_db_discovery);       /**< DB discovery module instance. */
NRF_SDH_BLE_OBSERVER(m_amts_ble_obs, BLE_AMTS_BLE_OBSERVER_PRIO, nrf_ble_amts_on_ble_evt, &m_amts);

static volatile bool currently_advertising = false;                                     /**< Are we currently advertising? */
static volatile bool currently_connected = false;
static volatile bool m_notif_enabled;
static volatile bool m_mtu_exchanged;
static volatile bool m_data_length_updated;
static volatile bool m_phy_updated;
static volatile bool m_conn_interval_configured;
static bool m_ble_stack_initalised;
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;    /**< Handle of the current BLE connection .*/
static uint8_t m_gap_role     = BLE_GAP_ROLE_INVALID;       /**< BLE role for this connection, see @ref BLE_GAP_ROLES */
static uint8_t m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;                           /**< Advertising handle used to identify an advertising set. */
static uint8_t m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];                            /**< Buffer for storing an encoded advertising set. */
static uint16_t connection_interval;

static volatile bool rx_pending;
static uint8_t * rx_request_buffer;
static uint32_t rx_request_length;
static volatile buffer_t rx_buffer_priv;
static volatile uint8_t  rx_buffer_pool[SYSHAL_BLE_RX_BUFFER_DEPTH * NRF_SDH_BLE_GATT_MAX_MTU_SIZE];

static volatile bool tx_pending;
static uint8_t * tx_request_buffer;
static uint32_t tx_request_length;
static uint32_t tx_bytes_sent;

// Test parameters.
typedef struct
{
    uint16_t        att_mtu;                    /**< GATT ATT MTU, in bytes. */
    uint16_t        advertising_interval;       /**< Advertising interval expressed in units of 0.625 ms. */
    uint16_t        conn_interval;              /**< Connection interval expressed in units of 1.25 ms. */
    ble_gap_phys_t  phys;                       /**< Preferred PHYs. */
    uint8_t         data_len;                   /**< Data length. */
    bool            conn_evt_len_ext_enabled;   /**< Connection event length extension status. */
} ble_params_t;

// Settings like ATT MTU size are set only once on the dummy board.
// Make sure that defaults are sensible.
static ble_params_t m_ble_params =
{
    .att_mtu                  = NRF_SDH_BLE_GATT_MAX_MTU_SIZE,
    .data_len                 = NRF_SDH_BLE_GAP_DATA_LENGTH,
    .advertising_interval     = ADV_INTERVAL_DEFAULT,
    .conn_interval            = CONN_INTERVAL_DEFAULT,
    .conn_evt_len_ext_enabled = true,
    .phys.tx_phys             = BLE_GAP_PHY_1MBPS,
    .phys.rx_phys             = BLE_GAP_PHY_1MBPS,
};

// Connection parameters requested for connection.
static ble_gap_conn_params_t m_conn_param =
{
    .min_conn_interval = CONN_INTERVAL_MIN,   // Minimum connection interval.
    .max_conn_interval = CONN_INTERVAL_MAX,   // Maximum connection interval.
    .slave_latency     = SLAVE_LATENCY,       // Slave latency.
    .conn_sup_timeout  = CONN_SUP_TIMEOUT     // Supervisory timeout.
};

/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data =
{
    .adv_data =
    {
        .p_data = m_enc_advdata,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = NULL,
        .len    = 0

    }
};

static void amts_evt_handler(nrf_ble_amts_evt_t evt);
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
static void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt);
static void ble_stack_init(void);
static void gap_params_init(void);
static void gatt_init(void);
static void gatt_mtu_set(uint16_t att_mtu);
static void advertising_data_set(void);
static void advertising_start(void);
static void advertising_stop(void);
static void on_ble_gap_evt_connected(ble_gap_evt_t const * p_gap_evt);
static void on_ble_gap_evt_disconnected(ble_gap_evt_t const * p_gap_evt);
static void qwr_init(void);
static void server_init(void);
static void data_len_set(uint8_t value);
static void conn_evt_len_ext_set(bool status);

#ifndef DEBUG_DISABLED
char const * phy_str(ble_gap_phys_t phys)
{
    static char const * str[] =
    {
        "1 Mbps",
        "2 Mbps",
        "Coded",
        "Unknown"
    };

    switch (phys.tx_phys)
    {
        case BLE_GAP_PHY_1MBPS:
            return str[0];

        case BLE_GAP_PHY_2MBPS:
            return str[1];

        case BLE_GAP_PHY_CODED:
            return str[2];

        default:
            return str[3];
    }
}
#endif

static uint8_t mode_phy(uint8_t mode)
{
    switch (mode)
    {
        case SYS_CONFIG_TAG_BLUETOOTH_PHY_MODE_1_MBPS:
            return BLE_GAP_PHY_1MBPS;
            break;
        case SYS_CONFIG_TAG_BLUETOOTH_PHY_MODE_2_MBPS:
            return BLE_GAP_PHY_2MBPS;
            break;
        case SYS_CONFIG_TAG_BLUETOOTH_PHY_MODE_CODED:
            return BLE_GAP_PHY_CODED;
            break;
        default:
            return BLE_GAP_PHY_1MBPS;
            break;
    }
}

/**@brief AMT server event handler. */
static void amts_evt_handler(nrf_ble_amts_evt_t evt)
{
    ret_code_t err_code;

    switch (evt.evt_type)
    {
        case NRF_BLE_AMTS_EVT_NOTIF_ENABLED:
        {
            DEBUG_PR_TRACE("Notifications enabled.");

            m_notif_enabled = true;

            if (m_ble_params.conn_interval != connection_interval)
            {
                DEBUG_PR_TRACE("Updating connection parameters..");
                m_conn_param.min_conn_interval = m_ble_params.conn_interval;
                m_conn_param.max_conn_interval = m_ble_params.conn_interval;
                err_code = sd_ble_gap_conn_param_update(m_conn_handle, &m_conn_param);

                if (err_code != NRF_SUCCESS)
                {
                    DEBUG_PR_ERROR("sd_ble_gap_conn_param_update() failed: 0x%08lX", err_code);
                }
            }
            else
            {
                m_conn_interval_configured = true;
            }
        } break;

        case NRF_BLE_AMTS_EVT_NOTIF_DISABLED:
        {
            DEBUG_PR_TRACE("Notifications disabled.");
        } break;

        case NRF_BLE_AMTS_EVT_TX_COMPLETE:
        {
            if (tx_bytes_sent >= tx_request_length)
            {
                DEBUG_PR_TRACE("BLE TX Complete");

                // If our transmission has fully completed then notify the application
                syshal_ble_event_t event =
                {
                    .error = SYSHAL_BLE_NO_ERROR,
                    .id = SYSHAL_BLE_EVENT_SEND_COMPLETE,
                    .send_complete = {
                        .length = tx_bytes_sent
                    }
                };

                // Reset pending buffer
                tx_pending = false;
                syshal_ble_event_handler(&event);
            }
            else
            {
                DEBUG_PR_TRACE("BLE packet sent, %lu bytes remaining", tx_request_length - tx_bytes_sent);
                // Send as much of our remaining data as we can
                uint16_t mtu_size = nrf_ble_amts_get_max_payload_size(&m_amts);
                uint16_t transmit_length = MIN(mtu_size, tx_request_length - tx_bytes_sent);

                nrf_ble_amts_send(&m_amts, &tx_request_buffer[tx_bytes_sent], transmit_length);
                tx_bytes_sent += transmit_length;
            }
        } break;

        case NRF_BLE_AMTS_EVT_RX_COMPLETE:
        {
            uint8_t * rx_ptr;
            if (!buffer_write(&rx_buffer_priv, (uintptr_t *)&rx_ptr))
            {
                DEBUG_PR_ERROR("BLE RX Buffer full, Data lost!");
                return;
            }

            // Copy the received data into our receive buffer pool
            memcpy(rx_ptr, evt.receive_complete.buffer, evt.receive_complete.length);

            buffer_write_advance(&rx_buffer_priv, evt.receive_complete.length);
        } break;

    }
}

/**@brief Function for handling BLE Stack events.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    uint32_t              err_code;
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_ADV_REPORT:
            //on_adv_report(&p_gap_evt->params.adv_report);
            break;

        case BLE_GAP_EVT_CONNECTED:
            currently_advertising = false;
            on_ble_gap_evt_connected(p_gap_evt);
            currently_connected = true;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            currently_connected = false;
            on_ble_gap_evt_disconnected(p_gap_evt);
            break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE:
        {
            m_conn_interval_configured = true;
            m_ble_params.conn_interval = p_gap_evt->params.conn_param_update.conn_params.min_conn_interval;
            DEBUG_PR_TRACE("Connection interval updated: 0x%x, 0x%x.",
                           p_gap_evt->params.conn_param_update.conn_params.min_conn_interval,
                           p_gap_evt->params.conn_param_update.conn_params.max_conn_interval);
        } break;

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        {
            // Accept parameters requested by the peer.
            ble_gap_conn_params_t params;
            params = p_gap_evt->params.conn_param_update_request.conn_params;
            err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle, &params);
            APP_ERROR_CHECK(err_code);

            DEBUG_PR_TRACE("Connection interval updated (upon request): 0x%x, 0x%x.",
                           p_gap_evt->params.conn_param_update_request.conn_params.min_conn_interval,
                           p_gap_evt->params.conn_param_update_request.conn_params.max_conn_interval);
        } break;

        case BLE_GATTS_EVT_SYS_ATTR_MISSING:
        {
            err_code = sd_ble_gatts_sys_attr_set(p_gap_evt->conn_handle, NULL, 0, 0);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GATTC_EVT_TIMEOUT: // Fallthrough.
        case BLE_GATTS_EVT_TIMEOUT:
        {
            DEBUG_PR_TRACE("GATT timeout, disconnecting.");
            err_code = sd_ble_gap_disconnect(m_conn_handle,
                                             BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_PHY_UPDATE:
        {
            ble_gap_evt_phy_update_t const * p_phy_evt = &p_ble_evt->evt.gap_evt.params.phy_update;

            if (p_phy_evt->status == BLE_HCI_STATUS_CODE_LMP_ERROR_TRANSACTION_COLLISION)
            {
                // Ignore LL collisions.
                DEBUG_PR_TRACE("LL transaction collision during PHY update.");
                break;
            }

            m_phy_updated = true;

#ifndef DEBUG_DISABLED
            ble_gap_phys_t phys = {0};
            phys.tx_phys = p_phy_evt->tx_phy;
            phys.rx_phys = p_phy_evt->rx_phy;
            DEBUG_PR_TRACE("PHY update %s. PHY set to %s.",
                           (p_phy_evt->status == BLE_HCI_STATUS_CODE_SUCCESS) ?
                           "accepted" : "rejected",
                           phy_str(phys));
#endif
        } break;

        case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
        {
            err_code = sd_ble_gap_phy_update(p_gap_evt->conn_handle, &m_ble_params.phys);
            APP_ERROR_CHECK(err_code);
        } break;

        case BLE_GAP_EVT_TIMEOUT: // This will occur if an advertising timeout is set
            currently_advertising = false;
            break;

        case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
            DEBUG_PR_TRACE("BLE_GAP_EVT_SEC_PARAMS_REQUEST");
            break;

        default:
            // No implementation needed.
            break;
    }
}

/**@brief Function for handling events from the GATT library. */
static void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    switch (p_evt->evt_id)
    {
        case NRF_BLE_GATT_EVT_ATT_MTU_UPDATED:
        {
            m_mtu_exchanged = true;
            DEBUG_PR_TRACE("ATT MTU exchange completed. MTU set to %u bytes.",
                           p_evt->params.att_mtu_effective);
        } break;

        case NRF_BLE_GATT_EVT_DATA_LENGTH_UPDATED:
        {
            m_data_length_updated = true;
            DEBUG_PR_TRACE("Data length updated to %u bytes.", p_evt->params.data_length);
        } break;
    }

    nrf_ble_amts_on_gatt_evt(&m_amts, p_evt);
}

/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    // Enable the softdevice if it is not already enabled
    if (!nrf_sdh_is_enabled())
    {
        err_code = nrf_sdh_enable_request();
        APP_ERROR_CHECK(err_code);
    }

    tx_pending = false;
    rx_pending = false;

    if (m_ble_stack_initalised)
        return;

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    if (err_code != NRF_SUCCESS)
    {
        DEBUG_PR_ERROR("nrf_sdh_ble_default_cfg_set() failed. Error code: 0x%08lX", err_code);
    }
    APP_ERROR_CHECK(err_code);

    // Overwrite the default write queue length for the BLE stack.
    ble_cfg_t ble_cfg;
    memset(&ble_cfg, 0, sizeof(ble_cfg));
    ble_cfg.conn_cfg.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;
    ble_cfg.conn_cfg.params.gatts_conn_cfg.hvn_tx_queue_size = 2; // Only allow a transmit queue of 2 packets
    err_code = sd_ble_cfg_set(BLE_CONN_CFG_GATTS, &ble_cfg, ram_start);
    if (err_code != NRF_SUCCESS)
    {
        DEBUG_PR_ERROR("sd_ble_cfg_set() failed when attempting to set queue size. Error code: 0x%08lX", err_code);
    }
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    if (err_code != NRF_SUCCESS)
    {
        DEBUG_PR_ERROR("nrf_sdh_ble_enable() failed. Error code: 0x%08lX", err_code);
    }
    APP_ERROR_CHECK(err_code);

    // Register a handler for BLE events.
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

/**@brief Function for initializing GAP parameters.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    ret_code_t              err_code;
    ble_gap_conn_sec_mode_t sec_mode;

    if (m_ble_stack_initalised)
        return;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (uint8_t const *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gap_ppcp_set(&m_conn_param);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing the GATT library. */
static void gatt_init(void)
{
    if (m_ble_stack_initalised)
        return;

    ret_code_t err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);
}

static void gatt_mtu_set(uint16_t att_mtu)
{
    ret_code_t err_code;

    if (m_ble_stack_initalised)
        return;

    m_ble_params.att_mtu = att_mtu;

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, att_mtu);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_ble_gatt_att_mtu_central_set(&m_gatt, att_mtu);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for setting up advertising data. */
static void advertising_data_set(void)
{
    ret_code_t ret;

    if (m_ble_stack_initalised)
        return;

    ble_gap_adv_params_t adv_params =
    {
        .properties =
        {
            .type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED,
        },
        .p_peer_addr   = NULL,
        .filter_policy = BLE_GAP_ADV_FP_ANY,
        .interval      = m_ble_params.advertising_interval, // Default value
        .duration      = 0,

        .primary_phy   = BLE_GAP_PHY_1MBPS, // Must be changed to connect in long range. (BLE_GAP_PHY_CODED)
        .secondary_phy = BLE_GAP_PHY_1MBPS,
    };

    // if adv interval has been set
    if (config.tag_bluetooth_advertising_interval->hdr.set)
    {
        adv_params.interval = config.tag_bluetooth_advertising_interval->contents.interval;
    }


    ble_advdata_t const adv_data =
    {
        .name_type          = BLE_ADVDATA_FULL_NAME,
        .flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE,
        .include_appearance = false,
    };

    ret = ble_advdata_encode(&adv_data, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(ret);

    ret = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &adv_params);
    APP_ERROR_CHECK(ret);
}

/**@brief Function for starting advertising. */
static void advertising_start(void)
{
    if (!currently_advertising)
    {
        DEBUG_PR_TRACE("Starting advertising.");

        ret_code_t err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);

        if (NRF_SUCCESS == err_code)
            currently_advertising = true;

        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for stopping advertising. */
static void advertising_stop(void)
{
    if (currently_advertising)
    {
        DEBUG_PR_TRACE("Stopping advertising.");

        ret_code_t err_code = sd_ble_gap_adv_stop(m_adv_handle);
        currently_advertising = false;

        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling BLE_GAP_EVT_CONNECTED events.
 * Save the connection handle and GAP role, then discover the peer DB.
 */
static void on_ble_gap_evt_connected(ble_gap_evt_t const * p_gap_evt)
{
    ret_code_t err_code;

    m_conn_handle = p_gap_evt->conn_handle;
    m_gap_role    = p_gap_evt->params.connected.role;


    m_ble_params.conn_interval = p_gap_evt->params.connected.conn_params.min_conn_interval;

    if (m_gap_role == BLE_GAP_ROLE_PERIPH)
    {
        DEBUG_PR_TRACE("Connected as a peripheral.");
    }
    else
    {
        APP_ERROR_CHECK(NRF_ERROR_INVALID_STATE); // We've somehow connected as a central device
    }

    // Stop advertising.
    (void) sd_ble_gap_adv_stop(m_adv_handle);

    // Assign connection handle to the Queued Write module.
    err_code = nrf_ble_qwr_conn_handle_assign(&m_qwr, m_conn_handle);
    APP_ERROR_CHECK(err_code);

    if (m_gap_role == BLE_GAP_ROLE_PERIPH)
    {
        DEBUG_PR_TRACE("Sending PHY Update, %s.", phy_str(m_ble_params.phys));

        err_code = sd_ble_gap_phy_update(p_gap_evt->conn_handle, &m_ble_params.phys);
        APP_ERROR_CHECK(err_code);
    }
}

/**@brief Function for handling BLE_GAP_EVT_DISCONNECTED events.
 * Unset the connection handle and terminate the test.
 */
static void on_ble_gap_evt_disconnected(ble_gap_evt_t const * p_gap_evt)
{
    m_conn_handle = BLE_CONN_HANDLE_INVALID;

    DEBUG_PR_TRACE("Disconnected: reason 0x%x.", p_gap_evt->params.disconnected.reason);

    m_notif_enabled            = false;
    m_mtu_exchanged            = false;
    m_data_length_updated      = false;
    m_phy_updated              = false;
    m_conn_interval_configured = false;

    // If we didn't terminate the connection ourselves
    if (BLE_HCI_LOCAL_HOST_TERMINATED_CONNECTION != p_gap_evt->params.disconnected.reason)
        advertising_start(); // Then start advertising again
}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**@brief Function for initializing the Queued Write module.
 */
static void qwr_init(void)
{
    ret_code_t         err_code;
    nrf_ble_qwr_init_t qwr_init_obj = {0};

    // Initialize Queued Write Module.
    qwr_init_obj.error_handler = nrf_qwr_error_handler;

    err_code = nrf_ble_qwr_init(&m_qwr, &qwr_init_obj);
    APP_ERROR_CHECK(err_code);
}

static void server_init(void)
{
    if (m_ble_stack_initalised)
        return;

    qwr_init();
    nrf_ble_amts_init(&m_amts, amts_evt_handler);
}

static void data_len_set(uint8_t value)
{
    ret_code_t err_code;
    err_code = nrf_ble_gatt_data_length_set(&m_gatt, BLE_CONN_HANDLE_INVALID, value);
    APP_ERROR_CHECK(err_code);

    m_ble_params.data_len = value;
}

static void conn_evt_len_ext_set(bool status)
{
    ret_code_t err_code;
    ble_opt_t  opt;

    memset(&opt, 0x00, sizeof(opt));
    opt.common_opt.conn_evt_ext.enable = status ? 1 : 0;

    err_code = sd_ble_opt_set(BLE_COMMON_OPT_CONN_EVT_EXT, &opt);
    APP_ERROR_CHECK(err_code);

    m_ble_params.conn_evt_len_ext_enabled = status;
}

int syshal_ble_init(syshal_ble_init_t ble_config)
{
    config = ble_config;

    buffer_init_policy(pool, &rx_buffer_priv,
                       (uintptr_t) &rx_buffer_pool[0],
                       sizeof(rx_buffer_pool), SYSHAL_BLE_RX_BUFFER_DEPTH);

    DEBUG_PR_TRACE("ble_stack_init");
    ble_stack_init();
    DEBUG_PR_TRACE("gap_params_init");
    gap_params_init();
    DEBUG_PR_TRACE("gatt_init");
    gatt_init();
    DEBUG_PR_TRACE("advertising_data_set");
    advertising_data_set();

    DEBUG_PR_TRACE("server_init");
    server_init();

    m_ble_stack_initalised = true;
    // Set a custom address if we have one
    if (config.tag_bluetooth_device_address->hdr.set)
    {
        ble_gap_addr_t address;

        for (uint32_t i = 0; i < BLE_GAP_ADDR_LEN; ++i)
            address.addr[i] = config.tag_bluetooth_device_address->contents.address[i];

        address.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
        address.addr[5] |= ((1 << 7) | (1 << 6)); // The top 2 bits must be set in Private address mode

        sd_ble_gap_addr_set(&address);
    }

    if (config.tag_bluetooth_connection_interval->hdr.set)
    {
        connection_interval = config.tag_bluetooth_connection_interval->contents.interval;
    }
    else
    {
        connection_interval = CONN_INTERVAL_DEFAULT;
    }

    if (config.tag_bluetooth_phy_mode->hdr.set)
    {
        m_ble_params.phys.tx_phys = mode_phy(config.tag_bluetooth_phy_mode->contents.mode);
        m_ble_params.phys.rx_phys = mode_phy(config.tag_bluetooth_phy_mode->contents.mode);
    }


    gatt_mtu_set(m_ble_params.att_mtu);
    conn_evt_len_ext_set(m_ble_params.conn_evt_len_ext_enabled);

    data_len_set(251);

    advertising_start();

    return SYSHAL_BLE_NO_ERROR;
}

int syshal_ble_term(void)
{
    // Stop advertising if we were
    advertising_stop();

    // Disconnect if we were connected
    if (m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        DEBUG_PR_TRACE("Disconnecting...");

        ret_code_t err_code;
        err_code = sd_ble_gap_disconnect(m_conn_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);

        if (err_code != NRF_SUCCESS)
            DEBUG_PR_ERROR("sd_ble_gap_disconnect() failed: 0x%08lX", err_code);
    }

    // Clear our receive buffer
    buffer_reset(&rx_buffer_priv);

    return SYSHAL_BLE_NO_ERROR;
}

int syshal_ble_set_mode(syshal_ble_mode_t mode)
{
    DEBUG_PR_WARN("%s Not implemented", __FUNCTION__);
    return SYSHAL_BLE_NO_ERROR;
}

int syshal_ble_get_mode(syshal_ble_mode_t *mode)
{
    DEBUG_PR_WARN("%s Not implemented", __FUNCTION__);
    return SYSHAL_BLE_NO_ERROR;
}

int syshal_ble_get_version(uint32_t *version)
{
//    ble_version_t ble_version;
//
//    if (!nrf_sdh_is_enabled())
//        return SYSHAL_BLE_ERROR_FAIL;
//
//    if (sd_ble_version_get(&ble_version))
//        return SYSHAL_BLE_ERROR_FAIL;
//
//    *version = ble_version.subversion_number;

    // Access the version using direct memory access
    // This is necessary as sd_ble_version_get() fails if
    // nrf_sdh_ble_enable() has not been called

    *version = *(uint16_t*) 0x0000300C;

    return SYSHAL_BLE_NO_ERROR;
}

int syshal_ble_send(uint8_t *buffer, uint32_t size)
{
    if (!currently_connected)
        return SYSHAL_BLE_ERROR_DISCONNECTED;

    // Only allow sending if notifications on the server have been enabled
    if (!m_notif_enabled)
        return SYSHAL_BLE_ERROR_FORBIDDEN;

    if (tx_pending)
        return SYSHAL_BLE_ERROR_BUSY;

    tx_pending = true;
    tx_request_length = size;
    tx_request_buffer = buffer;

    uint16_t mtu_size = nrf_ble_amts_get_max_payload_size(&m_amts);
    uint16_t transmit_length = MIN(mtu_size, tx_request_length);

    tx_bytes_sent = transmit_length;
    
    return nrf_ble_amts_send(&m_amts, tx_request_buffer, transmit_length);
}

int syshal_ble_receive(uint8_t *buffer, uint32_t size)
{
    if (!currently_connected)
        return SYSHAL_BLE_ERROR_DISCONNECTED;

    if (rx_pending)
        return SYSHAL_BLE_ERROR_BUSY;

    rx_pending = true;
    rx_request_buffer = buffer;
    rx_request_length = size;

    return SYSHAL_BLE_NO_ERROR;
}

int syshal_ble_tick(void)
{
    static bool currently_connected_prev = false;

    if (currently_connected_prev != currently_connected)
    {
        if (currently_connected)
        {
            // Notify the application that we have connected
            syshal_ble_event_t event;
            event.id = SYSHAL_BLE_EVENT_CONNECTED;
            syshal_ble_event_handler(&event);
        }
        else
        {
            // Notify the application that we have disconnected
            syshal_ble_event_t event;
            event.id = SYSHAL_BLE_EVENT_DISCONNECTED;
            syshal_ble_event_handler(&event);
        }
    }

    currently_connected_prev = currently_connected;

    if (rx_pending)
    {
        uint8_t * rx_ptr;
        uint32_t length = buffer_read(&rx_buffer_priv, (uintptr_t *)&rx_ptr);

        if (length)
        {
            rx_pending = false;
            uint32_t safe_length = MIN(length, rx_request_length);
            // WARN: If a packet contains more data then was requested the extra will be lost

            memcpy(rx_request_buffer, rx_ptr, safe_length);

            buffer_read_advance(&rx_buffer_priv, length); // Remove this packet from the receive buffer

            // Notify the application that we have sent the buffer
            syshal_ble_event_t event =
            {
                .error = SYSHAL_BLE_NO_ERROR,
                .id = SYSHAL_BLE_EVENT_RECEIVE_COMPLETE,
                .receive_complete = {
                    .length = safe_length
                }
            };

            syshal_ble_event_handler(&event);
        }
    }

    return SYSHAL_BLE_NO_ERROR;
}

__attribute__((weak)) int syshal_ble_event_handler(syshal_ble_event_t * event)
{
    DEBUG_PR_WARN("%s Not implemented", __FUNCTION__);
    return SYSHAL_BLE_NO_ERROR;
}
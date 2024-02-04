#include "cus_service.h"

#include "sdk_common.h"
#include "ble_srv_common.h"

																									 
#define BLE_CUSTOM_MAX_CHAR_LEN        (GATT_MTU_SIZE_DEFAULT - 3)        /**< Maximum length of the Custom Characteristic (in bytes). */




/**@brief Function for handling the @ref BLE_GAP_EVT_CONNECTED event from the S110 SoftDevice. */
static void on_connect(ble_cus_t * p_cus, ble_evt_t * p_ble_evt)
{
    p_cus->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
	
		ble_cus_evt_t evt;

		evt.evt_type = BLE_CUS_EVT_CONNECTED;

		p_cus->evt_handler(p_cus, &evt);
}


/**@brief Function for handling the @ref BLE_GAP_EVT_DISCONNECTED event from the S110 SoftDevice. */
static void on_disconnect(ble_cus_t * p_cus, ble_evt_t * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_cus->conn_handle = BLE_CONN_HANDLE_INVALID;
	
		ble_cus_evt_t evt;

		evt.evt_type = BLE_CUS_EVT_DISCONNECTED;

		p_cus->evt_handler(p_cus, &evt);
}


/**@brief Function for handling the @ref BLE_GATTS_EVT_WRITE event from the S110 SoftDevice. */
static void on_write(ble_cus_t * p_cus, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

		// Check if the Custom value CCCD is written to and that the value is the appropriate length, i.e 2 bytes.
    if ((p_evt_write->handle == p_cus->notify_custom_value_handles.cccd_handle)
				&& (p_evt_write->len == 2))
		
    {
				ble_cus_evt_t evt;
				// CCCD written, call application event handler
        if (ble_srv_is_notification_enabled(p_evt_write->data))
        {
            p_cus->is_notification_enabled = true;
						evt.evt_type = BLE_CUS_EVT_NOTIFICATION_ENABLED;
        }
        else
        {
            p_cus->is_notification_enabled = false;
						evt.evt_type = BLE_CUS_EVT_NOTIFICATION_DISABLED;
        }
				// Call the application event handler.
				p_cus->evt_handler(p_cus, &evt);
    }
    else if ((p_evt_write->handle == p_cus->write_custom_value_handles.value_handle)
             && (p_cus->data_handler != NULL))
		
    {
        p_cus->data_handler(p_cus, p_evt_write->data, p_evt_write->len);
    }
    else
    {
        // Do Nothing. This event is not relevant for this service.
    }
}

static void on_read(ble_cus_t * p_cus, ble_evt_t const * p_ble_evt)
{
	ble_cus_evt_t                 evt;
	ble_gatts_evt_read_t const * p_evt_read = &p_ble_evt->evt.gatts_evt.params.authorize_request.request.read;
	if ((p_evt_read->handle == p_cus->read_custom_value_handles.value_handle) &&
							 (p_cus->data_handler != NULL))	
	{
        evt.evt_type                  = BLE_CUS_EVT_READ;
        p_cus->evt_handler(p_cus, &evt);	
	}
}


static uint32_t custom_value_char_write_add(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init)
{
		uint32_t            err_code;
		ble_gatts_char_md_t char_md;
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
		
		// -------------------------
	
		memset(&char_md, 0, sizeof(char_md));
		
		char_md.char_props.read   = 0;	// need for read with response
    char_md.char_props.write  = 1;
		// --- Configure Notify ----
    char_md.char_props.notify = 0; 
		// -------------------------
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = NULL; // Only Notify
    char_md.p_sccd_md         = NULL;
		memset(&attr_md, 0, sizeof(attr_md));
	
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
		
		// A other way to set up permissions
//    attr_md.write_perm = p_cus_init->custom_value_char_attr_md.write_perm;
		//  The .vloc option is set to BLE_GATTS_VLOC_STACK as we want the characteristic to be stored in the SoftDevice RAM section 
		//  and not in the Application RAM section.
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 0;		// need for read with response
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 1;		// 0: Get full size of attribute characteristic --> BLE_CUSTOM_MAX_CHAR_LEN
															// 1: Get fit enough with data size 
		
		ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = p_cus_init->char_write_uuid;
		
		memset(&attr_char_value, 0, sizeof(attr_char_value));
		
		attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(uint8_t);
    attr_char_value.init_offs = 0;
		// Maximum lenth that can contain all characters from nRF Connect app send to the device 
		// For example, attr_char_value.max_len   = sizeof(uint8_t); ==> only receive 1 byte.
		// 							attr_char_value.max_len   = BLE_NUS_MAX_TX_CHAR_LEN; ==> receive > 1 byte
    attr_char_value.max_len   = BLE_CUSTOM_MAX_CHAR_LEN;
		
		
		err_code = sd_ble_gatts_characteristic_add(p_cus->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_cus->write_custom_value_handles);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
	
}


static uint32_t custom_value_char_read_add(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init)
{
		uint32_t            err_code;
		ble_gatts_char_md_t char_md;
		ble_gatts_attr_md_t cccd_md;	// cccd: Client Characteristic Configuration Descriptor, need to be provide the READ and WRITE permission by command 'BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm)' and 'BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);'
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
		
	
		// --- Configure Notify ----
	  memset(&cccd_md, 0, sizeof(cccd_md));

    //  Read  operation on Cccd should be possible without authentication.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);		// When enabling NOTIFY, we must enable the READ permission, if not is error
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);	// When enabling NOTIFY, we must enable the WRITE permission, if not is error
    
    cccd_md.vloc       = BLE_GATTS_VLOC_STACK;
		// -------------------------
	
		memset(&char_md, 0, sizeof(char_md));
		
		char_md.char_props.read   = 1;	// need for read with response
    char_md.char_props.write  = 0;
		// --- Configure Notify ----
    char_md.char_props.notify = 0; 
		// -------------------------
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md; // Only Notify
    char_md.p_sccd_md         = NULL;
	
		memset(&attr_md, 0, sizeof(attr_md));
	
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);	// When enabling READ properties, we must enable the READ permission, if not is error
		
		// A other way to set up permissions
//		attr_md.read_perm  = p_cus_init->custom_value_char_attr_md.read_perm;
//    attr_md.write_perm = p_cus_init->custom_value_char_attr_md.write_perm;
		//  The .vloc option is set to BLE_GATTS_VLOC_STACK as we want the characteristic to be stored in the SoftDevice RAM section 
		//  and not in the Application RAM section.
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 1;		// need for read with response
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 1;		// 0: Get full size of attribute characteristic --> BLE_CUSTOM_MAX_CHAR_LEN
															// 1: Get fit enough with data size 
		
		ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = p_cus_init->char_read_uuid;
		
		memset(&attr_char_value, 0, sizeof(attr_char_value));
		
		attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(uint8_t);
    attr_char_value.init_offs = 0;
		// Maximum lenth that can contain all characters from nRF Connect app send to the device 
		// For example, attr_char_value.max_len   = sizeof(uint8_t); ==> only receive 1 byte.
		// 							attr_char_value.max_len   = BLE_NUS_MAX_TX_CHAR_LEN; ==> receive > 1 byte
    attr_char_value.max_len   = BLE_CUSTOM_MAX_CHAR_LEN;
		
		
		err_code = sd_ble_gatts_characteristic_add(p_cus->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_cus->read_custom_value_handles);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
	
}

static uint32_t custom_value_char_notify_add(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init)
{
		uint32_t            err_code;
		ble_gatts_char_md_t char_md;
		ble_gatts_attr_md_t cccd_md;	// cccd: Client Characteristic Configuration Descriptor, need to be provide the READ and WRITE permission by command 'BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm)' and 'BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);'
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;
		
	
		// --- Configure Notify ----
	  memset(&cccd_md, 0, sizeof(cccd_md));

    //  Read  operation on Cccd should be possible without authentication.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);		// When enabling NOTIFY, we must enable the READ permission, if not is error
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);	// When enabling NOTIFY, we must enable the WRITE permission, if not is error
    
    cccd_md.vloc       = BLE_GATTS_VLOC_STACK;
		// -------------------------
	
		memset(&char_md, 0, sizeof(char_md));
		
		char_md.char_props.read   = 0;	// need for read with response
    char_md.char_props.write  = 0;
		// --- Configure Notify ----
    char_md.char_props.notify = 1; 
		// -------------------------
    char_md.p_char_user_desc  = NULL;
    char_md.p_char_pf         = NULL;
    char_md.p_user_desc_md    = NULL;
    char_md.p_cccd_md         = &cccd_md; // Only Notify and Read permission
    char_md.p_sccd_md         = NULL;
	
		memset(&attr_md, 0, sizeof(attr_md));
	
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);
		BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);	// When enabling READ properties, we must enable the READ permission, if not is error
		
		// A other way to set up permissions
//		attr_md.read_perm  = p_cus_init->custom_value_char_attr_md.read_perm;
//    attr_md.write_perm = p_cus_init->custom_value_char_attr_md.write_perm;
		//  The .vloc option is set to BLE_GATTS_VLOC_STACK as we want the characteristic to be stored in the SoftDevice RAM section 
		//  and not in the Application RAM section.
    attr_md.vloc       = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth    = 1;		// need for read with response
    attr_md.wr_auth    = 0;
    attr_md.vlen       = 1;		// 0: Get full size of attribute characteristic --> BLE_CUSTOM_MAX_CHAR_LEN
															// 1: Get fit enough with data size 
		
		ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = p_cus_init->char_notify_uuid;
		
		memset(&attr_char_value, 0, sizeof(attr_char_value));
		
		attr_char_value.p_uuid    = &ble_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.init_len  = sizeof(uint8_t);
    attr_char_value.init_offs = 0;
		// Maximum lenth that can contain all characters from nRF Connect app send to the device 
		// For example, attr_char_value.max_len   = sizeof(uint8_t); ==> only receive 1 byte.
		// 							attr_char_value.max_len   = BLE_NUS_MAX_TX_CHAR_LEN; ==> receive > 1 byte
    attr_char_value.max_len   = BLE_CUSTOM_MAX_CHAR_LEN;
		
		
		err_code = sd_ble_gatts_characteristic_add(p_cus->service_handle, &char_md,
                                               &attr_char_value,
                                               &p_cus->notify_custom_value_handles);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
	
}


void ble_cus_on_ble_evt(ble_cus_t * p_cus, ble_evt_t * p_ble_evt)
{
    if ((p_cus == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_cus, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_cus, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_cus, p_ble_evt);
            break;

				case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
						on_read(p_cus, p_ble_evt);
				
        default:
            // No implementation needed.
            break;
    }
}


uint32_t ble_cus_init(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init)
{
    uint32_t      err_code;
    ble_uuid_t    ble_uuid;

    VERIFY_PARAM_NOT_NULL(p_cus);
    VERIFY_PARAM_NOT_NULL(p_cus_init);

    // Initialize function pointer to point to 'main.c' to get data of chacracteristic value attribute
    p_cus->conn_handle             = BLE_CONN_HANDLE_INVALID;
    p_cus->data_handler            = p_cus_init->data_handler;
    p_cus->is_notification_enabled = false;

		// Initialize function pointer to point to 'main.c' to get event of chacracteristic value attribute
		p_cus->evt_handler               = p_cus_init->evt_handler;
	
    
    // Add a custom base UUID.
		ble_uuid128_t base_uuid = {CUSTOM_SERVICE_UUID_BASE};
		err_code =  sd_ble_uuid_vs_add(&base_uuid, &p_cus->uuid_type);
		VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_cus->uuid_type;
    ble_uuid.uuid = p_cus_init->service_uuid;

    // Add the service.
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_cus->service_handle);
    VERIFY_SUCCESS(err_code);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
		
		// Add the Characteristic with WRITE permission
		err_code = custom_value_char_write_add(p_cus, p_cus_init);
		
		if (err_code != NRF_SUCCESS)
		{
				return err_code;
		}
		
		// Add the Characteristic with READ permission
		err_code = custom_value_char_read_add(p_cus, p_cus_init);
		
		if (err_code != NRF_SUCCESS)
		{
				return err_code;
		}
		
		// Add the Characteristic with NOTIFY permission
		err_code = custom_value_char_notify_add(p_cus, p_cus_init);
		
		if (err_code != NRF_SUCCESS)
		{
				return err_code;
		}
		
    return err_code;
}

// This function send a string data to nRF Mobile App
uint32_t ble_cus_string_send(ble_cus_t * p_cus, uint8_t * p_string, uint16_t length)
{
		// ------------- Updated -------------
		
		if ((p_cus->conn_handle == BLE_CONN_HANDLE_INVALID) || (!p_cus->is_notification_enabled))
    {
        return NRF_ERROR_INVALID_STATE;
    }

    if (length > BLE_CUSTOM_MAX_DATA_LEN)
    {
        return NRF_ERROR_INVALID_PARAM;
    }
	
		ble_gatts_value_t gatts_value;
		
		// Initialize value struct.
		memset(&gatts_value, 0, sizeof(gatts_value));

		gatts_value.len     = length;
		gatts_value.offset  = 0;
		gatts_value.p_value = p_string;

		// Update database.
		sd_ble_gatts_value_set(p_cus->conn_handle,
																				p_cus->read_custom_value_handles.value_handle,
																				&gatts_value);
	
	
		ble_gatts_hvx_params_t hvx_params;

		memset(&hvx_params, 0, sizeof(hvx_params));

		hvx_params.handle = p_cus->read_custom_value_handles.value_handle;
		hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
		hvx_params.offset = gatts_value.offset;
		hvx_params.p_len  = &gatts_value.len;
		hvx_params.p_data = gatts_value.p_value;

		return sd_ble_gatts_hvx(p_cus->conn_handle, &hvx_params);
	
		// -----------------------------------
	
	
		// Following NUS Service
			
//    ble_gatts_hvx_params_t hvx_params;

//    VERIFY_PARAM_NOT_NULL(p_cus);

//    if ((p_cus->conn_handle == BLE_CONN_HANDLE_INVALID) || (!p_cus->is_notification_enabled))
//    {
//        return NRF_ERROR_INVALID_STATE;
//    }

//    if (length > BLE_CUSTOM_MAX_DATA_LEN)
//    {
//        return NRF_ERROR_INVALID_PARAM;
//    }

//    memset(&hvx_params, 0, sizeof(hvx_params));

//    hvx_params.p_data = p_string;
//    hvx_params.p_len  = &length;
//    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

//    return sd_ble_gatts_hvx(p_cus->conn_handle, &hvx_params);
}

// This function only send 1 byte data to nRF Mobile App
uint32_t ble_cus_custom_value_update(ble_cus_t * p_cus, uint8_t custom_value)
{
    if (p_cus == NULL)
    {
        return NRF_ERROR_NULL;
    }
		
		uint32_t err_code = NRF_SUCCESS;
		ble_gatts_value_t gatts_value;
		
		// Initialize value struct.
		memset(&gatts_value, 0, sizeof(gatts_value));

		gatts_value.len     = sizeof(uint8_t);
		gatts_value.offset  = 0;
		gatts_value.p_value = &custom_value;

		// Update database.
		err_code = sd_ble_gatts_value_set(p_cus->conn_handle,
																				p_cus->notify_custom_value_handles.value_handle,
																				&gatts_value);
		if (err_code != NRF_SUCCESS)
		{
				return err_code;
		}
		
		
		// Send value if connected and notifying.
		if ((p_cus->conn_handle != BLE_CONN_HANDLE_INVALID)) 
		{
				ble_gatts_hvx_params_t hvx_params;

				memset(&hvx_params, 0, sizeof(hvx_params));

				hvx_params.handle = p_cus->notify_custom_value_handles.value_handle;
				hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
				hvx_params.offset = gatts_value.offset;
				hvx_params.p_len  = &gatts_value.len;
				hvx_params.p_data = gatts_value.p_value;

				err_code = sd_ble_gatts_hvx(p_cus->conn_handle, &hvx_params);
		}
		else
		{
				err_code = NRF_ERROR_INVALID_STATE;
		}

		return err_code;
}





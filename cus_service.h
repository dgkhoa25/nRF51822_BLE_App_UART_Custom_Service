#ifndef __CUS_SERVICE_H_
#define __CUS_SERVICE_H_

#include "ble.h"
#include "ble_srv_common.h"

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
	extern "C" {
#endif

#define CUSTOM_SERVICE_UUID_BASE                  {0xBC, 0x8A, 0xBF, 0x45, 0xCA, 0x05, 0x50, 0xBA, \
																									 0x40, 0x42, 0xB0, 0x00, 0xC9, 0xAD, 0x64, 0xF3}

#define BLE_UUID_CUSTOM_SERVICE 		0x1400
#define BLE_UUID_CUSTOM_SERVICE_2 	0x1500

#define BLE_UUID_CUSTOM_VAL_CHA_WRITE			 	0x1401
#define BLE_UUID_CUSTOM_VAL_CHA_READ				0x1402
#define BLE_UUID_CUSTOM_VAL_CHA_NOTIFY			0x1403

#define BLE_UUID_CUSTOM_VAL_CHA_WRITE_2			0x1501
#define BLE_UUID_CUSTOM_VAL_CHA_READ_2			0x1502
#define BLE_UUID_CUSTOM_VAL_CHA_NOTIFY_2		0x1503


#define BLE_CUSTOM_MAX_DATA_LEN (GATT_MTU_SIZE_DEFAULT - 3) /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */

/* Forward declaration of the ble_nus_t type. */
typedef struct ble_cus_s ble_cus_t;

/**@brief Nordic UART Service event handler type. */
typedef void (*ble_cus_data_handler_t) (ble_cus_t * p_cus, uint8_t * p_data, uint16_t length);

/**@brief Custom Service event type. */
typedef enum
{
    BLE_CUS_EVT_NOTIFICATION_ENABLED,                             /**< Custom value notification enabled event. */
    BLE_CUS_EVT_NOTIFICATION_DISABLED,                            /**< Custom value notification disabled event. */
    BLE_CUS_EVT_DISCONNECTED,
    BLE_CUS_EVT_CONNECTED,
		BLE_CUS_EVT_READ
} ble_cus_evt_type_t;


/**@brief Custom Service event. */
typedef struct
{
    ble_cus_evt_type_t evt_type;                                  /**< Type of event. */
} ble_cus_evt_t;

/**@brief Custom Service event handler type. */
typedef void (*ble_cus_evt_handler_t) (ble_cus_t * p_cus, ble_cus_evt_t * p_evt);




/**@brief Nordic UART Service initialization structure.
 *
 * @details This structure contains the initialization information for the service. The application
 * must fill this structure and pass it to the service using the @ref ble_nus_init
 *          function.
 */
typedef struct
{
		ble_cus_evt_handler_t         evt_handler;                    /**< Event handler to be called for handling events in the Custom Service. */
		uint16_t											service_uuid;
		uint16_t 											char_write_uuid;
		uint16_t 											char_read_uuid;
		uint16_t 											char_notify_uuid;
		uint8_t                       initial_custom_value;           /**< Initial custom value */
		ble_srv_cccd_security_mode_t  custom_value_char_attr_md;     	/**< Initial security level for Custom characteristics attribute */
    ble_cus_data_handler_t 				data_handler; 									/**< Event handler to be called for handling received data. */
} ble_cus_init_t;

/**@brief Nordic UART Service structure.
 *
 * @details This structure contains status information related to the service.
 */
struct ble_cus_s
{
		ble_cus_evt_handler_t    evt_handler;                    /**< Event handler to be called for handling events in the Custom Service. */
    uint8_t                  uuid_type;              
    uint16_t                 service_handle;        
		// Here, only initialize 1 Custom value characteristics, if there are 2 (> 1) Custom characteristics,
		// Must initialize 2 (>1) 'ble_gatts_char_handles_t'
		// Eg,.  ble_gatts_char_handles_t  custom_value_handles_1;
		//			 ble_gatts_char_handles_t  custom_value_handles_2;
		//			 ble_gatts_char_handles_t  ...;
    ble_gatts_char_handles_t write_custom_value_handles;
		ble_gatts_char_handles_t read_custom_value_handles;
		ble_gatts_char_handles_t notify_custom_value_handles;
    uint16_t                 conn_handle;            
    bool                     is_notification_enabled; 
    ble_cus_data_handler_t   data_handler; 									/**< Event handler to be called for handling received data. */
};

/**@brief Function for initializing the Nordic UART Service.
 *
 * @param[out] p_nus      Nordic UART Service structure. This structure must be supplied
 *                        by the application. It is initialized by this function and will
 *                        later be used to identify this particular service instance.
 * @param[in] p_nus_init  Information needed to initialize the service.
 *
 * @retval NRF_SUCCESS If the service was successfully initialized. Otherwise, an error code is returned.
 * @retval NRF_ERROR_NULL If either of the pointers p_nus or p_nus_init is NULL.
 */
uint32_t ble_cus_init(ble_cus_t * p_cus, const ble_cus_init_t * p_cus_init);

/**@brief Function for handling the Nordic UART Service's BLE events.
 *
 * @details The Nordic UART Service expects the application to call this function each time an
 * event is received from the SoftDevice. This function processes the event if it
 * is relevant and calls the Nordic UART Service event handler of the
 * application if necessary.
 *
 * @param[in] p_nus       Nordic UART Service structure.
 * @param[in] p_ble_evt   Event received from the SoftDevice.
 */
void ble_cus_on_ble_evt(ble_cus_t * p_cus, ble_evt_t * p_ble_evt);

/**@brief Function for sending a string to the peer.
 *
 * @details This function sends the input string as an RX characteristic notification to the
 *          peer.
 *
 * @param[in] p_nus       Pointer to the Nordic UART Service structure.
 * @param[in] p_string    String to be sent.
 * @param[in] length      Length of the string.
 *
 * @retval NRF_SUCCESS If the string was sent successfully. Otherwise, an error code is returned.
 */
uint32_t ble_cus_string_send(ble_cus_t * p_cus, uint8_t * p_string, uint16_t length);

/**@brief Function for updating the custom value.
 *
 * @details The application calls this function when the cutom value should be updated. If
 *          notification has been enabled, the custom value characteristic is sent to the client.
 *
 * @note 
 *       
 * @param[in]   p_cus          Custom Service structure.
 * @param[in]   Custom value 
 *
 * @return      NRF_SUCCESS on success, otherwise an error code.
 */
uint32_t ble_cus_custom_value_update(ble_cus_t * p_cus, uint8_t custom_value);

#ifdef __cplusplus
}
#endif

#endif

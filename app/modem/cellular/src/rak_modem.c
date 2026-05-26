#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/drivers/cellular.h>
#include <zephyr/drivers/regulator.h>

#include "rak_modem.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem, LOG_LEVEL_INF);

// Modem Devices
static const struct device *mdm = DEVICE_DT_GET(DT_ALIAS(modem));
static const struct device *mdm_uart = DEVICE_DT_GET(DT_ALIAS(modem_uart));
static const struct device *gps = DEVICE_DT_GET(DT_NODELABEL(gps_pwr));

struct net_if *iface = NULL;

static const char *modem_cellular_registration_status_str(enum cellular_registration_status status)
{
	switch (status) {
	case CELLULAR_REGISTRATION_NOT_REGISTERED:
		return "NOT_REGISTERED";
	case CELLULAR_REGISTRATION_REGISTERED_HOME:
		return "REGISTERED_HOME";
	case CELLULAR_REGISTRATION_SEARCHING:
		return "SEARCHING";
	case CELLULAR_REGISTRATION_DENIED:
		return "DENIED";
	case CELLULAR_REGISTRATION_UNKNOWN:
		return "UNKNOWN";
	case CELLULAR_REGISTRATION_REGISTERED_ROAMING:
		return "ROAMING";
	case CELLULAR_REGISTRATION_SMS_ONLY_HOME:
		return "SMS_ONLY_HOME";
	case CELLULAR_REGISTRATION_SMS_ONLY_ROAMING:
		return "SMS_ONLY_ROAMING";
	case CELLULAR_REGISTRATION_EMERGENCY_ONLY:
		return "EMERGENCY_ONLY";
	case CELLULAR_REGISTRATION_CSFB_NOT_PREFERRED_HOME:
		return "CSFB_NOT_PREFERRED_HOME";
	case CELLULAR_REGISTRATION_CSFB_NOT_PREFERRED_ROAMING:
		return "CSFB_NOT_PREFERRED_ROAMING";
	case CELLULAR_REGISTRATION_RLOS:
		return "RLOS";
	}

	return "";
}

int rak_modem_init()
{
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));
	if (iface == NULL) {
		LOG_ERR("Failed to init iface.");
		return -EIO;
	}

	return 0;
}

int rak_modem_connect(int timeout_sec)
{
	int ret = net_mgmt_event_wait_on_iface(iface, NET_EVENT_L4_CONNECTED, NULL, NULL, NULL,
					       K_SECONDS(timeout_sec));
	if (ret != 0) {
		LOG_ERR("L4 was not connected in time: %d", ret);
		return ret;
	}

	LOG_INF("Successfully connected to LTE-M network.");

	return ret;
}

int rak_modem_disable_gps_power()
{
	int ret = regulator_disable(gps);
	if (ret) {
		LOG_ERR("Failed to suspend gps: %d", ret);
	}

	return ret;
}

void rak_modem_resume()
{
	(void)pm_device_action_run(mdm_uart, PM_DEVICE_ACTION_RESUME);
	(void)pm_device_action_run(mdm, PM_DEVICE_ACTION_RESUME);
}

void rak_modem_suspend()
{
	(void)pm_device_action_run(mdm, PM_DEVICE_ACTION_SUSPEND);
	(void)pm_device_action_run(mdm_uart, PM_DEVICE_ACTION_SUSPEND);
}

int rak_modem_if_up()
{
	return net_if_up(iface);
}

int rak_modem_if_down()
{
	return net_if_down(iface);
}

int rak_modem_get_cellular_info(rak_modem_info *modem_info)
{
	int ret;

	ret = cellular_get_registration_status(mdm, CELLULAR_ACCESS_TECHNOLOGY_E_UTRAN,
					       &modem_info->reg_status);
	if (ret) {
		LOG_ERR("Failed to get registration status: %d", ret);
	}

	// ret = cellular_get_signal(mdm, CELLULAR_SIGNAL_RSSI, &modem_info->rssi);
	// if (ret) {
	// 	LOG_ERR("Failed to get signal: %d", ret);
	// }

	ret = cellular_get_modem_info(mdm, CELLULAR_MODEM_INFO_IMEI, modem_info->imei,
				      sizeof(modem_info->imei));
	if (ret) {
		LOG_ERR("Failed to get modem imei: %d", ret);
	}

	ret = cellular_get_modem_info(mdm, CELLULAR_MODEM_INFO_MODEL_ID, modem_info->model_id,
				      sizeof(modem_info->model_id));
	if (ret) {
		LOG_ERR("Failed to get model id: %d", ret);
	}

	ret = cellular_get_modem_info(mdm, CELLULAR_MODEM_INFO_MANUFACTURER,
				      modem_info->manufacturer, sizeof(modem_info->manufacturer));
	if (ret) {
		LOG_ERR("Failed to get manufacturer: %d", ret);
	}

	ret = cellular_get_modem_info(mdm, CELLULAR_MODEM_INFO_SIM_IMSI, modem_info->imsi,
				      sizeof(modem_info->imsi));
	if (ret) {
		LOG_ERR("Failed to get imsi: %d", ret);
	}

	ret = cellular_get_modem_info(mdm, CELLULAR_MODEM_INFO_SIM_ICCID, modem_info->iccid,
				      sizeof(modem_info->iccid));
	if (ret) {
		LOG_ERR("Failed to get iccid: %d", ret);
	}

	ret = cellular_get_modem_info(mdm, CELLULAR_MODEM_INFO_FW_VERSION, modem_info->fw_version,
				      sizeof(modem_info->fw_version));
	if (ret) {
		LOG_ERR("Failed to get fw version: %d", ret);
	}

	return 0;
}

void rak_modem_print_info(rak_modem_info info)
{
	LOG_INF("================================");
	LOG_INF("IMEI: %s", info.imei);
	LOG_INF("MODEL_ID: %s", info.model_id);
	LOG_INF("MANUFACTURER: %s", info.manufacturer);
	LOG_INF("FW_VERSION: %s", info.fw_version);
	LOG_INF("SIM_IMSI: %s", info.imsi);
	LOG_INF("SIM_ICCID: %s", info.iccid);
	LOG_INF("RSSI: %d", info.rssi);
	LOG_INF("REGISTRATION: %s", modem_cellular_registration_status_str(info.reg_status));
	LOG_INF("================================");
}

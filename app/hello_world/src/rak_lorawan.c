#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rak_lorawan, LOG_LEVEL_DBG);

/* Customize based on network configuration */
#define LORAWAN_APP_KEY                                                                            \
	{                                                                                          \
		0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09,      \
			0xCF, 0x4F, 0x3C                                                           \
	}
#define LORAWAN_DEV_EUI                                                                            \
	{                                                                                          \
		0xDD, 0xEE, 0xAA, 0xDD, 0xBB, 0xEE, 0xEE, 0xFF                                     \
	}
#define LORAWAN_JOIN_EUI                                                                           \
	{                                                                                          \
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00                                     \
	}

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS (3 * 60 * 1000)

#define LORAWAN_THREAD_STACKSIZE 4096
#define LORAWAN_THREAD_PRIORITY  10

/*
 * Get LoRa device
 */
static const struct device *lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));

int rak_lorawan_init()
{
	int ret = 0;

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s: device not ready.", lora_dev->name);
		return -1;
	}

	ret = lorawan_start();
	if (ret < 0) {
		LOG_ERR("lorawan_start failed: %d", ret);
	}

	return ret;
}

static void lorawan_dl_callback(uint8_t port, uint8_t flags, int16_t rssi, int8_t snr, uint8_t len,
				const uint8_t *hex_data)
{
	LOG_INF("Port %d, Pending %d, RSSI %ddB, SNR %ddBm, Time %d", port,
		flags & LORAWAN_DATA_PENDING, rssi, snr, !!(flags & LORAWAN_TIME_UPDATED));
	if (hex_data) {
		LOG_HEXDUMP_INF(hex_data, len, "Payload: ");
	}
}

static void lorawan_datarate_changed(enum lorawan_datarate dr)
{
	uint8_t unused, max_size;

	lorawan_get_payload_sizes(&unused, &max_size);
	LOG_INF("New Datarate: DR_%d, Max Payload %d", dr, max_size);
}

void lorawan_thread_handler(void)
{
	static const char buffer[256] = "This is test message!";
	struct lorawan_join_config join_cfg;
	uint8_t dev_eui[] = LORAWAN_DEV_EUI;
	uint8_t join_eui[] = LORAWAN_JOIN_EUI;
	uint8_t app_key[] = LORAWAN_APP_KEY;
	int ret;

	struct lorawan_downlink_cb downlink_cb = {.port = LW_RECV_PORT_ANY,
						  .cb = lorawan_dl_callback};

	lorawan_register_downlink_callback(&downlink_cb);
	lorawan_register_dr_changed_callback(lorawan_datarate_changed);

	join_cfg.mode = LORAWAN_ACT_OTAA;
	join_cfg.dev_eui = dev_eui;
	join_cfg.otaa.join_eui = join_eui;
	join_cfg.otaa.app_key = app_key;
	join_cfg.otaa.nwk_key = app_key;
	join_cfg.otaa.dev_nonce = 0u;

	LOG_INF("Joining network over OTAA");

	do {
		ret = lorawan_join(&join_cfg);
		if (ret < 0) {
			LOG_ERR("lorawan_join to network failed: %d", ret);
		}

		if (ret < 0) {
			LOG_INF("Waiting for retrying to join...");
			k_sleep(K_MSEC(5000));
		}
	} while (ret != 0);

	lorawan_enable_adr(true);

	LOG_INF("Joined network");

	while (1) {
		ret = lorawan_send(2, (uint8_t *)buffer, strlen(buffer), LORAWAN_MSG_CONFIRMED);

		/*
		 * Note: The stack may return -EAGAIN if the provided data
		 * length exceeds the maximum possible one for the region and
		 * datarate. But since we are just sending the same data here,
		 * we'll just continue.
		 */
		if (ret < 0) {
			LOG_ERR("lorawan_send failed: %d", ret);
		} else {
			LOG_INF("Data sent!");
		}

		k_sleep(K_MSEC(SLEEP_TIME_MS));
	}
}
K_THREAD_DEFINE(lorawan_thread_id, LORAWAN_THREAD_STACKSIZE, lorawan_thread_handler, NULL, NULL,
		NULL, LORAWAN_THREAD_PRIORITY, 0, -1);

void rak_lorawan_thread_start()
{
	k_thread_start(lorawan_thread_id);
}

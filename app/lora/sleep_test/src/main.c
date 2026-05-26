#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/irq.h>

#if defined(CONFIG_BOARD_RAK3112)
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#endif

#include "rak_ble_peripheral.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

/*
 * Get LED GPIO
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(green_led), gpios);

static void led_init()
{
	if (!gpio_is_ready_dt(&led)) {
		LOG_ERR("LED is not ready!");
		return;
	}

	int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret) {
		LOG_ERR("Failed to configure LED!");
	}

	ret = gpio_pin_set_dt(&led, 1); // ON
	if (ret) {
		LOG_ERR("Failed to control LED!");
	}
	k_msleep(1000);
	ret = gpio_pin_set_dt(&led, 0); // OFF
	if (ret) {
		LOG_ERR("Failed to control LED!");
	}

	ret = gpio_pin_configure_dt(&led, GPIO_DISCONNECTED);
	if (ret) {
		LOG_ERR("Failed to disconnect LED!");
	}
}

#if defined(CONFIG_BOARD_RAK11160)
/*
 * Get ESP8684 enable regulator
 */
static const struct device *esp = DEVICE_DT_GET(DT_NODELABEL(esp8684_enable));
#endif

/*
 * Get LoRa device
 */
static const struct device *lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));

int main(void)
{
#if defined(CONFIG_BT_PERIPHERAL)
	int ret = rak_ble_peripheral_init();
	if (ret) {
		LOG_ERR("Failed to init ble peripheral: %d", ret);
		return -1;
	}
#endif

#if defined(CONFIG_BOARD_RAK11160)
	// Suspend ESP MCU
	int ret = regulator_disable(esp);
	if (ret) {
		LOG_ERR("Failed to suspend esp: %d", ret);
	}
#endif

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s: device not ready.", lora_dev->name);
		return -1;
	}

	led_init();

	LOG_INF("Sleep Test! %s", CONFIG_BOARD);

#if defined(CONFIG_BOARD_RAK3112)
	if (esp_sleep_get_wakeup_causes() & BIT(ESP_SLEEP_WAKEUP_TIMER)) {
		LOG_INF("Wake up from timer.");
	} else {
		LOG_INF("Not a deep sleep reset.");
	}

	/* Enabling timer wakeup for 30 seconds */
	esp_sleep_enable_timer_wakeup(30 * 1000000);

	/* Hold related pins on required levels during deep sleep */
	uint8_t dio1_pin = 47;
	uint8_t reset_pin = 8;
	uint8_t cs_pin = 7;
	rtc_gpio_pulldown_en((gpio_num_t)dio1_pin);
	rtc_gpio_pullup_en((gpio_num_t)reset_pin);
	rtc_gpio_pullup_en((gpio_num_t)cs_pin);

	LOG_INF("Powering off...");

	k_busy_wait(1000000);

	(void)irq_lock();

	/* For timer wakeup you must leave RTC peripherals ON */
	esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
	esp_deep_sleep_start();

	/* Never reached here! */
#endif

	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}

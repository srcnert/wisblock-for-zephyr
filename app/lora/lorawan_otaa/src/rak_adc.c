#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rak_adc, LOG_LEVEL_DBG);

#if defined(CONFIG_BOARD_RAK3112) || defined(CONFIG_BOARD_RAK11160)
double correction_ratio = 1.1;
#endif

#if defined(CONFIG_BOARD_RAK3172)
double correction_ratio = 1.1;
#endif

#if defined(CONFIG_BOARD_RAK4631) || defined(CONFIG_BOARD_RAK3362)
double correction_ratio = 1.1;
#endif

#if defined(CONFIG_BOARD_RAK11720)
double correction_ratio = (2.13) * (1.3);
#endif

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

static double vref = 0.0;
static uint8_t resolution = 0;
static uint16_t buf = 0;
static struct adc_sequence sequence = {
	.buffer = &buf,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(buf),
#if defined(CONFIG_BOARD_RAK3172)
	.calibrate = true,
#endif
};

int rak_adc_init()
{
	int ret;

	/* Configure adc channel. */
	if (!adc_is_ready_dt(&adc_channel)) {
		LOG_ERR("ADC controller device %s not ready", adc_channel.dev->name);
		return -1;
	}

	ret = adc_channel_setup_dt(&adc_channel);
	if (ret < 0) {
		LOG_ERR("Failed to setup channel: %d", ret);
		return ret;
	}

	int32_t vref_mv = (int32_t)adc_ref_internal(adc_channel.dev);
	enum adc_gain gain = adc_channel.channel_cfg.gain;
	adc_gain_invert(gain, &vref_mv);
	vref = (double)(vref_mv / 1000.0);
	resolution = adc_channel.resolution;

	LOG_INF("ADC Vref: %d", vref_mv);
	LOG_INF("ADC Resolution: %d", resolution);

	(void)adc_sequence_init_dt(&adc_channel, &sequence);

	return ret;
}

int rak_adc_sample(double *value)
{
	int ret = -1;
	int32_t val;

	ret = adc_read_dt(&adc_channel, &sequence);
	if (ret < 0) {
		LOG_ERR("Failed to read: %d", ret);
		return ret;
	}

	val = (int32_t)buf;
	LOG_DBG("ADC: %" PRId32, val);

	/*
	If rak19007 board is used as a base board,
	- no battery, USB connected
		* meaningless value, you read the Vout of the TP4054 in none-charging mode
	- battery, USB connected
		* meaningless value, you read the charging voltage of the TP4054
	- battery, USB not connected
		* battery voltage reading

	Due to this, RTT logging is used, please do not connect your usb line
	to get correct ADC data!

	Please connect a lipo battery to your RAK19007 board!
	*/

	*value = correction_ratio *
		 ((vref * (((double)val) / ((double)pow(2, resolution))) * (5.0)) / (3.0));
	return ret;
}

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
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#if !DT_NODE_EXISTS(DT_PATH(zephyr_user)) || !DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
#error "No suitable devicetree overlay specified"
#endif

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)};

uint16_t buf;
struct adc_sequence sequence = {
	.buffer = &buf,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(buf),
};

int main(void)
{
	int ret;

	/* Configure channels individually prior to sampling. */
	for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
		if (!adc_is_ready_dt(&adc_channels[i])) {
			LOG_ERR("ADC controller device %s not ready", adc_channels[i].dev->name);
			return 0;
		}

		ret = adc_channel_setup_dt(&adc_channels[i]);
		if (ret < 0) {
			LOG_ERR("Could not setup channel #%d (%d)", i, ret);
			return 0;
		}
	}

	/* As an example, just lipo battery voltage will be read.
	 * If you will use more than one channel, please read these parameters
	 * for every channel.
	 */
#if defined(CONFIG_BOARD_RAK3112)
	/* https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32s3/api-reference/peripherals/adc.html
	 */
	int32_t vref_mv = 3100;
#else
	int32_t vref_mv = (int32_t)adc_ref_internal(adc_channels[0].dev);
	enum adc_gain gain = adc_channels[0].channel_cfg.gain;
	adc_gain_invert(gain, &vref_mv);
#endif
	double vref = (double)(vref_mv / 1000.0);
	uint8_t resolution = adc_channels[0].resolution;

	LOG_INF("Vref: %d", vref_mv);
	LOG_INF("Resolution: %d", resolution);

	while (1) {
		/* ADC reading */
		for (size_t i = 0U; i < ARRAY_SIZE(adc_channels); i++) {
			int32_t val;

#if defined(CONFIG_BOARD_RAK3172) || defined(CONFIG_BOARD_RAK11160)
			LOG_INF("Calibrate ADC");
			sequence.calibrate = true;
#endif

			LOG_INF("%s, channel %d ", adc_channels[i].dev->name,
				adc_channels[i].channel_id);

			(void)adc_sequence_init_dt(&adc_channels[i], &sequence);

			ret = adc_read_dt(&adc_channels[i], &sequence);
			if (ret < 0) {
				LOG_ERR("Could not read (%d)", ret);
				continue;
			}

			val = (int32_t)buf;
			LOG_INF("ADC: %" PRId32, val);

			/*
			If rak19007 board is used as a base board,
			- no battery, USB connected
				* meaningless value, you read the Vout of the TP4054 in
			none-charging mode
			- battery, USB connected
				* meaningless value, you read the charging voltage of the TP4054
			- battery, USB not connected
				* battery voltage reading

			Due to this, RTT/UART logging is used, please do not connect your usb line
			to get correct ADC data!

			Please connect a lipo battery to your RAK19007 board!
			*/
			double bat_lvl = 0.0;

#if defined(CONFIG_BOARD_RAK3112)
			bat_lvl = ((vref * (((double)val) / ((double)pow(2, resolution))) * (5.0)) /
				   (3.0));
#endif

#if defined(CONFIG_BOARD_RAK3172) || defined(CONFIG_BOARD_RAK11160)
			bat_lvl = ((vref * (((double)val) / ((double)pow(2, resolution))) * (5.0)) /
				   (3.0));
#endif

#if defined(CONFIG_BOARD_RAK4631) || defined(CONFIG_BOARD_RAK3362)
			bat_lvl = ((vref * (((double)val) / ((double)pow(2, resolution))) * (5.0)) /
				   (3.0));
#endif

#if defined(CONFIG_BOARD_RAK5010)
			bat_lvl = ((vref * (((double)val) / ((double)pow(2, resolution))) * (5.0)) /
				   (2.0)) +
				  0.3;
#endif

#if defined(CONFIG_BOARD_RAK11720)
			// Apollo3's adc range is 2V!
			// RAK5005-O/RAK19007 Divider Resistor: 2.5 / 1.5
			// RAK11722 Divider Resistor: 1.3 / 1.0
			// Calibration: X * 2.02 + 0.21
			bat_lvl = ((vref * (((double)val) / ((double)pow(2, resolution))) * 1.3 *
				    (5.0)) /
				   (3.0)) *
					  2.02 +
				  0.21;
#endif

			LOG_INF("%s, V_BAT = %" PRId32 " mV", CONFIG_BOARD,
				(int32_t)(bat_lvl * 1000.0));
		}

		k_sleep(K_MSEC(1000));
	}

	return 0;
}

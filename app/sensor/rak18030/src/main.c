#include <zephyr/kernel.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/regulator.h>

#define AUDIO_FREQ       16000
#define NUMBER_OF_CHAN   1
#define SAMPLE_BIT_WIDTH 16
#define BYTES_PER_SAMPLE (SAMPLE_BIT_WIDTH / 8)
#define READ_TIMEOUT_MS  1000

#define BLOCK_SIZE_SAMPLES 1600 /* 100ms audio data */
#define BLOCK_SIZE_BYTES   (BLOCK_SIZE_SAMPLES * BYTES_PER_SAMPLE)
#define BLOCK_COUNT        10

#define RECORD_DURATION_SEC   6
#define RECORD_DURATION_INDEX (RECORD_DURATION_SEC * 10)

/* Define a memory slab to manage audio buffers */
K_MEM_SLAB_DEFINE(mem_slab, BLOCK_SIZE_BYTES, BLOCK_COUNT, 4);

struct pcm_stream_cfg stream = {
	.pcm_rate = AUDIO_FREQ,
	.pcm_width = SAMPLE_BIT_WIDTH,
	.block_size = BLOCK_SIZE_BYTES,
	.mem_slab = &mem_slab,
};

struct dmic_cfg cfg = {
	.io =
		{
			/* These fields can be used to limit the PDM clock
			 * configurations that the driver is allowed to use
			 * to those supported by the microphone.
			 */
			.min_pdm_clk_freq = 1000000,
			.max_pdm_clk_freq = 3500000,
			.min_pdm_clk_dc = 40,
			.max_pdm_clk_dc = 60,
		},
	.streams = &stream,
	.channel =
		{
			.req_num_chan = NUMBER_OF_CHAN,
			.req_num_streams = 1,
		},
};

const struct device *const dmic_dev = DEVICE_DT_GET(DT_NODELABEL(dmic_dev));

/*
 * Get LoRa device
 */
static const struct device *lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));

int main(void)
{
	int ret;
	uint16_t index = 0;

	if (!device_is_ready(lora_dev)) {
		printk("%s: device not ready.\n", lora_dev->name);
		return -1;
	}

	printk("DMIC sample: %s\n", CONFIG_BOARD);

	printk("Waiting for mic to stabilize...\n");
	k_msleep(100);

	if (!device_is_ready(dmic_dev)) {
		printk("%s is not ready\n", dmic_dev->name);
		return -EIO;
	}

	printk("PCM output rate: %u, channels: %u\n", cfg.streams[0].pcm_rate,
	       cfg.channel.req_num_chan);

	/* Select your mic channel */
	cfg.channel.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_RIGHT);

	ret = dmic_configure(dmic_dev, &cfg);
	if (ret < 0) {
		printk("Failed to configure the driver: %d\n", ret);
		return ret;
	}

	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);
	if (ret < 0) {
		printk("START trigger failed: %d\n", ret);
		return ret;
	}

	printk("Start of mic record\n");

	while (1) {
		void *buffer;
		uint32_t size;

		ret = dmic_read(dmic_dev, 0, &buffer, &size, READ_TIMEOUT_MS);
		if (ret < 0) {
			printk("%d - read failed: %d\n", index, ret);
			continue;
		}

		int16_t *pcm_out = (int16_t *)buffer;

		for (int j = 0; j < size / 2; j++) {
			printk("%d,", pcm_out[j]);
			if ((j + 1) % 16 == 0) {
				printk("\n");
			}
		}

		k_mem_slab_free(&mem_slab, buffer);

		index++;

		if (index > RECORD_DURATION_INDEX) {
			printk("End of mic record\n");
			break;
		}
	}

	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_STOP);
	if (ret < 0) {
		printk("STOP trigger failed: %d\n", ret);
	}

	printk("Exiting\n");
	return 0;
}

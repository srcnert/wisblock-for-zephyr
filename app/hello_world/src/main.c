#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/fs/fs.h>
#include <ff.h>

#include "rak_lorawan.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

FATFS fatfs;
#define MOUNT_POINT "/SD:"
uint16_t index = 1;

static struct fs_mount_t fatfs_mnt = {
	.type = FS_FATFS,
	.mnt_point = MOUNT_POINT,
	.fs_data = &fatfs,
};

static int sd_card_early_init(void)
{
	static const char *disk_pdrv = "SD";
	int ret;

	ret = disk_access_init(disk_pdrv);
	if (ret != 0) {
		LOG_ERR("Failed to init disk: %d", ret);
		return ret;
	}

	ret = fs_mount(&fatfs_mnt);
	if (ret != 0) {
		LOG_ERR("Failed to mount disk: %d", ret);
		return ret;
	}

	return 0;
}

SYS_INIT(sd_card_early_init, APPLICATION, 99);

int main(void)
{
	int ret;

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	ret = rak_lorawan_init();
	if (ret != 0) {
		LOG_ERR("Failed to start lorawan: %d", ret);
		return 0;
	}

	rak_lorawan_thread_start();

	LOG_INF("LoRaWAN OTAA example started @%s", CONFIG_BOARD);

	while (true) {
		LOG_INF("Hello World! %s - %d", CONFIG_BOARD, index);
		index++;
		k_sleep(K_SECONDS(1));
	}
}

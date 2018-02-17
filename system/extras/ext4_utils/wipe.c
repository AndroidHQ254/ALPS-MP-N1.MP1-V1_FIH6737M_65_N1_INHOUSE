/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ext4_utils.h"
#include "wipe.h"

#if WIPE_IS_SUPPORTED

#if defined(__linux__)

#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef BLKDISCARD
#define BLKDISCARD _IO(0x12,119)
#endif

#ifndef BLKSECDISCARD
#define BLKSECDISCARD _IO(0x12,125)
#endif

#define MT_NORMAL_BOOT      0
#define MT_META_BOOT        1
#define MT_FACTORY_BOOT     4
#define MT_ATE_FACTORY_BOOT 6

static int get_boot_mode(void)
{
	int fd;
	int s;
	char boot_mode[4] = {'0'};

	fd = open("/sys/class/BOOT/BOOT/boot/boot_mode", O_RDONLY);
	if (fd < 0) {
		warn("fail to open: %s\n", "/sys/class/BOOT/BOOT/boot/boot_mode");
		return 0;
	}

	s = read(fd, (void *)&boot_mode, sizeof(boot_mode) - 1);
	close(fd);

	if (s <= 0) {
		warn("could not read boot mode sys file\n");
		return 0;
	}

	boot_mode[s] = '\0';
	return atoi((const char*)boot_mode);
}

int wipe_block_device(int fd, s64 len)
{
	u64 range[2];
	int ret;
	int bootmode;

	if (!is_block_device_fd(fd)) {
		// Wiping only makes sense on a block device.
		return 0;
	}

	range[0] = 0;
	range[1] = len;

	bootmode=get_boot_mode();

	if ((bootmode == MT_META_BOOT) ||
	    (bootmode == MT_FACTORY_BOOT) ||
	    (bootmode == MT_ATE_FACTORY_BOOT)) {
		ret = -1;
	}
	else {
		ret = ioctl(fd, BLKSECDISCARD, &range);
	}

	if (ret < 0) {
		range[0] = 0;
		range[1] = len;
		ret = ioctl(fd, BLKDISCARD, &range);
		if (ret < 0) {
			warn("Discard failed\n");
			return 1;
		} else {
			warn("Wipe via secure discard failed, used discard instead\n");
			return 0;
		}
	}

	return 0;
}

#else  /* __linux__ */
#error "Missing block device wiping implementation for this platform!"
#endif

#else  /* WIPE_IS_SUPPORTED */

int wipe_block_device(int fd, s64 len)
{
	/* Wiping is not supported on this platform. */
	return 1;
}

#endif  /* WIPE_IS_SUPPORTED */

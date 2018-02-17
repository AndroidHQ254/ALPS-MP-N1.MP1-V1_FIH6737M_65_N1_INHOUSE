/*
 * Copyright (C) 2015 Mediatek Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

#include <linux/kdev_t.h>

#define LOG_TAG "Vold"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <selinux/selinux.h>

#include <logwrap/logwrap.h>

#include <unistd.h>
#include <dirent.h>

#include "Ntfs.h"
#include "Utils.h"
#include "VoldUtil.h"

using android::base::StringPrintf;

namespace android {
namespace vold {
namespace ntfs {

static const char* kFsckPath = "/vendor/bin/ntfsfix";
static const char* kMountPath = "/vendor/bin/ntfs-3g";

bool IsSupported() {
    bool ret;
    ret = (access(kFsckPath, X_OK) == 0) &&
        (access(kMountPath, X_OK) == 0);
    SLOGI("ntfs::IsSupported() = %s", ret?"true":"false");
    return ret;
}

status_t Check(const std::string& source) {
    if (access(kFsckPath, X_OK)) {
        SLOGW("Skipping fs checks\n");
        return 0;
    }

    int rc = 0;
    do {
        std::string exec;
        exec = kFsckPath;
        exec = exec + " " + source;

        rc = system(exec.c_str());

        switch(rc) {
        case 0:
            SLOGI("Filesystem check completed OK");
            return 0;
        default:
            SLOGE("Filesystem check failed (unknown exit code %d)", rc);
            errno = EIO;
            return -1;
        }
    } while (0);

    return 0;
}

status_t WaitForFile(const char *file)
{
    int retry=0;
    int ret;
    struct stat buf;

    while (retry<10) {
       SLOGI("ntfs::WaitForFile(): %s, try %d", file, retry);
       if (stat(file, &buf)==0) return 0;
       if (errno!=ENOENT) {
           SLOGE("ntfs::WaitForFile(): %s, error: %s", file, strerror(errno));
           return -1;
       }
       retry++;
       usleep(100000);
    }
    SLOGI("ntfs::WaitForFile(): %s, timeout", file);
    return -1;
}

status_t Mount(const std::string& source, const std::string& target, bool ro,
        bool remount, bool executable, int ownerUid, int ownerGid, int permMask,
        bool createLost) {
    int rc;
    char mountData[255];

    const char* c_source = source.c_str();
    const char* c_target = target.c_str();

    if (remount)
        SLOGE("ntfs::Mount() remount is not supported");

    /*
     * Note: This is a temporary hack. If the sampling profiler is enabled,
     * we make the SD card world-writable so any process can write snapshots.
     *
     * TODO: Remove this code once we have a drop box in system_server.
     */
    char value[PROPERTY_VALUE_MAX];
    property_get("persist.sampling_profiler", value, "");
    if (value[0] == '1') {
        SLOGW("The SD card is world-writable because the"
            " 'persist.sampling_profiler' system property is set to '1'.");
        permMask = 0;
    }

retry:
    sprintf(mountData,
            "nodev,nosuid,uid=%d,gid=%d,fmask=%o,dmask=%o%s%s",
            ownerUid, ownerGid, permMask, permMask,
            ro?"ro,":"", executable?"":"noexec");


    do {
        const char *c_blk_dev;
        std::string blk_dev;
        std::string exec;

        blk_dev = source;
        c_blk_dev = blk_dev.c_str();

        if (WaitForFile(c_blk_dev))
            return -1;

        exec = kMountPath;
        exec = exec + " -o " + mountData + " " + blk_dev + " " + target;
        SLOGI("ntfs::Mount() execute command %s", exec.c_str());
        rc = system(exec.c_str());

        SLOGI("ntfs::Mount(%s,%s,%s) = %d %s", c_blk_dev, c_target,mountData, rc, (rc)?strerror(errno):"");
        if (!ro && rc && errno == EROFS) {
            SLOGE("Mount %s as ntfs was failed, %d, %s", c_blk_dev, errno, strerror(errno));
            SLOGE("%s appears to be a read only filesystem - retrying mount RO", c_blk_dev);
            ro=true;
            goto retry;
        }
    }
    while(0);

    if (rc == 0 && createLost) {
        char *lost_path;
        asprintf(&lost_path, "%s/LOST.DIR", c_target);
        if (access(lost_path, F_OK)) {
            /*
             * Create a LOST.DIR in the root so we have somewhere to put
             * lost cluster chains (fsck_msdos doesn't currently do this)
             */
            if (mkdir(lost_path, 0755)) {
                SLOGE("Unable to create LOST.DIR (%s)", strerror(errno));
            }
        }
        free(lost_path);
    }
    return rc;
}

status_t Format(const std::string& source, unsigned int numSectors) {
    SLOGW("ntfs::Format() is not supported");
    return 0;
}

}  // namespace exfat
}  // namespace vold
}  // namespace android

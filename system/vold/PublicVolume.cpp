/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "fs/Vfat.h"
#include "fs/Exfat.h"
#include "fs/Ntfs.h"
#include "PublicVolume.h"
#include "Utils.h"
#include "VolumeManager.h"
#include "ResponseCode.h"

#include <android-base/stringprintf.h>
#include <android-base/logging.h>
#include <cutils/fs.h>
#include <private/android_filesystem_config.h>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

using android::base::StringPrintf;

namespace android {
namespace vold {

static const char* kFusePath = "/system/bin/sdcard";

static const char* kAsecPath = "/mnt/secure/asec";

static const char* kToken = "/";

#define NTFS_3G_PATH    "/vendor/bin/ntfs-3g"

static const unsigned int kMajorBlockMmc = 179;
#ifdef MTK_EMMC_SUPPORT
    static const unsigned int kMinorBlockEMMC = 0;
#else
    static const unsigned int kMinorBlockEMMC = 65535;
#endif


PublicVolume::PublicVolume(dev_t device) :
        VolumeBase(Type::kPublic), mDevice(device), mFusePid(0) {
    setId(StringPrintf("public:%u,%u", major(device), minor(device)));
    mDevPath = StringPrintf("/dev/block/vold/%s", getId().c_str());
    LOG(DEBUG) << "PublicVolume(" << getId() << ") is newed";
}

PublicVolume::~PublicVolume() {
}

status_t PublicVolume::readMetadata() {
    status_t res = ReadMetadataUntrusted(mDevPath, mFsType, mFsUuid, mFsLabel);
    notifyEvent(ResponseCode::VolumeFsTypeChanged, mFsType);
    notifyEvent(ResponseCode::VolumeFsUuidChanged, mFsUuid);
    notifyEvent(ResponseCode::VolumeFsLabelChanged, mFsLabel);
    return res;
}

status_t PublicVolume::initAsecStage() {
    std::string legacyPath(mRawPath + "/android_secure");
    std::string securePath(mRawPath + "/.android_secure");

    // Recover legacy secure path
    if (!access(legacyPath.c_str(), R_OK | X_OK)
            && access(securePath.c_str(), R_OK | X_OK)) {
        if (rename(legacyPath.c_str(), securePath.c_str())) {
            PLOG(WARNING) << getId() << " failed to rename legacy ASEC dir";
        }
    }

    if (TEMP_FAILURE_RETRY(mkdir(securePath.c_str(), 0700))) {
        if (errno != EEXIST) {
            PLOG(WARNING) << getId() << " creating ASEC stage failed";
            return -errno;
        }
    }

    BindMount(securePath, kAsecPath);

    return OK;
}

status_t PublicVolume::doCreate() {
    return CreateDeviceNode(mDevPath, mDevice);
}

status_t PublicVolume::doDestroy() {
    return DestroyDeviceNode(mDevPath);
}

status_t PublicVolume::doMount() {
    // TODO: expand to support mounting other filesystems
    LOG(VERBOSE) << "PublicVolume::doMount()";
    readMetadata();
    LOG(VERBOSE) << getId() << " mFsType: " << mFsType;

    if (mFsType != "vfat" && mFsType != "exfat" && mFsType != "ntfs") {
        LOG(ERROR) << getId() << " unsupported filesystem " << mFsType;
        return -EIO;
    }
    if (mFsType == "vfat")
        if (vfat::Check(mDevPath)) {
            LOG(ERROR) << getId() << " failed filesystem check";
            return -EIO;
        }
    if (mFsType == "exfat") {
        if (!exfat::IsSupported()) {
            PLOG(ERROR) << "exfat is not supported on this system";
            return -EIO;
        }
        if (exfat::Check(mDevPath)) {
            LOG(ERROR) << getId() << " failed filesystem check";
            return -EIO;
        }
    }
    if (mFsType == "ntfs") {
        if (!ntfs::IsSupported()) {
            PLOG(ERROR) << "ntfs is not supported on this system";
            return -EIO;
        }
        if (ntfs::Check(mDevPath)) {
            LOG(ERROR) << getId() << " failed filesystem check";
            return -EIO;
        }
    }

    // Use UUID as stable name, if available
    std::string stableName = getId();
    if (!mFsUuid.empty()) {
        stableName = mFsUuid;
    }

    mRawPath = StringPrintf("/mnt/media_rw/%s", stableName.c_str());

    mFuseDefault = StringPrintf("/mnt/runtime/default/%s", stableName.c_str());
    mFuseRead = StringPrintf("/mnt/runtime/read/%s", stableName.c_str());
    mFuseWrite = StringPrintf("/mnt/runtime/write/%s", stableName.c_str());

    setInternalPath(mRawPath);
    if (getMountFlags() & MountFlags::kVisible) {
        setPath(StringPrintf("/storage/%s", stableName.c_str()));
    } else {
        setPath(mRawPath);
    }
    if (isMountpointMounted(getPath().c_str())) {
        PLOG(ERROR) << "skip mount";
        return OK;
     }

    if (fs_prepare_dir(mRawPath.c_str(), 0700, AID_ROOT, AID_ROOT)) {
        PLOG(ERROR) << getId() << " failed to create mount points";
        return -errno;
    }

    if (mFsType == "ntfs") {
        if (ntfs::Mount(mDevPath, mRawPath, false, false, false,
                AID_MEDIA_RW, AID_MEDIA_RW, 0007, true)) {
            PLOG(ERROR) << getId() << " ntfs: failed to mount " << mDevPath;
            return -EIO;
        }
        else
            LOG(VERBOSE) << "ntfs: mount succeed " << mDevPath;
    }
    else if (mFsType == "exfat") {
        if (exfat::Mount(mDevPath, mRawPath, false, false, false,
                AID_MEDIA_RW, AID_MEDIA_RW, 0007, true)) {
            PLOG(ERROR) << getId() << " exfat: failed to mount " << mDevPath;
            return -EIO;
        }
        else
            LOG(VERBOSE) << "exfat: mount succeed " << mDevPath;
    }
    else if (mFsType == "vfat") {
        if (vfat::Mount(mDevPath, mRawPath, false, false, false,
                AID_MEDIA_RW, AID_MEDIA_RW, 0007, true)) {
            PLOG(ERROR) << getId() << " vfat: failed to mount " << mDevPath;
            return -EIO;
        }
    }

    if (getMountFlags() & MountFlags::kPrimary) {
        initAsecStage();
    }

    if (!(getMountFlags() & MountFlags::kVisible)) {
        // Not visible to apps, so no need to spin up FUSE
        return OK;
    }

    if (fs_prepare_dir(mFuseDefault.c_str(), 0700, AID_ROOT, AID_ROOT) ||
            fs_prepare_dir(mFuseRead.c_str(), 0700, AID_ROOT, AID_ROOT) ||
            fs_prepare_dir(mFuseWrite.c_str(), 0700, AID_ROOT, AID_ROOT)) {
        PLOG(ERROR) << getId() << " failed to create FUSE mount points";
        return -errno;
    }

    dev_t before = GetDevice(mFuseWrite);

    if (!(mFusePid = fork())) {
        if (getMountFlags() & MountFlags::kPrimary) {
            if (execl(kFusePath, kFusePath,
                    "-u", "1023", // AID_MEDIA_RW
                    "-g", "1023", // AID_MEDIA_RW
                    "-U", std::to_string(getMountUserId()).c_str(),
                    "-w",
                    mRawPath.c_str(),
                    stableName.c_str(),
                    NULL)) {
                PLOG(ERROR) << "Failed to exec";
            }
        } else {
            if (execl(kFusePath, kFusePath,
                    "-u", "1023", // AID_MEDIA_RW
                    "-g", "1023", // AID_MEDIA_RW
                    "-U", std::to_string(getMountUserId()).c_str(),
                    mRawPath.c_str(),
                    stableName.c_str(),
                    NULL)) {
                PLOG(ERROR) << "Failed to exec";
            }
        }

        LOG(ERROR) << "FUSE exiting";
        _exit(1);
    }

    if (mFusePid == -1) {
        PLOG(ERROR) << getId() << " failed to fork";
        return -errno;
    }

    while (before == GetDevice(mFuseWrite)) {
        LOG(VERBOSE) << "Waiting for FUSE to spin up...";
        usleep(50000); // 50ms
    }

    return OK;
}

status_t PublicVolume::doUnmount() {
    // Unmount the storage before we kill the FUSE process. If we kill
    // the FUSE process first, most file system operations will return
    // ENOTCONN until the unmount completes. This is an exotic and unusual
    // error code and might cause broken behaviour in applications.
    KillProcessesUsingPath(getPath());

    ForceUnmount(kAsecPath);

    ForceUnmount(getPath());
    ForceUnmount(mFuseDefault);
    ForceUnmount(mFuseRead);
    ForceUnmount(mFuseWrite);
    ForceUnmount(mRawPath);

    if (mFusePid > 0) {
        kill(mFusePid, SIGTERM);
        TEMP_FAILURE_RETRY(waitpid(mFusePid, nullptr, 0));
        mFusePid = 0;
    }

    rmdir(mFuseDefault.c_str());
    rmdir(mFuseRead.c_str());
    rmdir(mFuseWrite.c_str());
    rmdir(mRawPath.c_str());

    mFuseDefault.clear();
    mFuseRead.clear();
    mFuseWrite.clear();
    mRawPath.clear();

    return OK;
}

status_t PublicVolume::doFormat(const std::string& fsType) {
    if (fsType == "vfat" || fsType == "auto") {
        if (WipeBlockDevice(mDevPath) != OK) {
            LOG(WARNING) << getId() << " failed to wipe";
        }
        if (vfat::Format(mDevPath, 0)) {
            LOG(ERROR) << getId() << " failed to format";
            return -errno;
        }
    } else {
        LOG(ERROR) << "Unsupported filesystem " << fsType;
        return -EINVAL;
    }

    return OK;
}

#include "VolumeManager.h"
#define MASS_STORAGE_FILE_PATH  "/sys/class/android_usb/android0/f_mass_storage/lun/file"
#define MASS_STORAGE_FILE1_PATH  "/sys/class/android_usb/android0/f_mass_storage/lun1/file"
#define DIRTY_RATIO_PATH "/proc/sys/vm/dirty_ratio"

status_t PublicVolume::doShare() {
    VolumeManager *vm =VolumeManager::Instance();
    if ((major(mDevice) == 0) && (minor(mDevice) == 0)) {
        // This volume does not support raw disk access
        errno = EINVAL;
        return -1;
    }

    int fd;
    strcpy(mUmsFilePath, MASS_STORAGE_FILE_PATH);
    if (vm->mUmsSharingCount == 0) {
        strcpy(mUmsFilePath, MASS_STORAGE_FILE_PATH);
    }
    else if (vm->mUmsSharingCount == 1) {
        strcpy(mUmsFilePath, MASS_STORAGE_FILE1_PATH);
    }
    else {
        LOG(ERROR) << getId() << " no more ums lunfile, count=" << vm->mUmsSharingCount;
        errno = EINVAL;
        return -1;
    }

    auto disk = vm->findDisk(getDiskId());
    if (disk == nullptr) {
        LOG(ERROR) << getId() << " cannot find disk, " << getDiskId();
        errno = EINVAL;
        return -1;
    }
    if (disk->mShared) {
        LOG(ERROR) << getId() << " disk, " << getDiskId() << ", is already shared";
        return OK;
    }

    if ((fd = open(mUmsFilePath, O_WRONLY)) < 0) {
        LOG(ERROR) << getId() << " unable to open ums lunfile, " << mUmsFilePath << ", " <<  strerror(errno);
        return -1;
    }

    /* only share the partition for phone storage */
    if ( major(disk->getDevice()) == kMajorBlockMmc && minor(disk->getDevice()) == kMinorBlockEMMC) {
         if (write(fd, mDevPath.c_str(), strlen(mDevPath.c_str())) < 0) {
             LOG(ERROR) << getId() << " unable to write 'partition' to ums lunfile, " <<  strerror(errno);
             close(fd);
             return -1;
         }
    }
    else {
        if (write(fd, disk->getDevPath().c_str(), strlen(disk->getDevPath().c_str())) < 0) {
            LOG(ERROR) << getId() << " unable to write 'disk' to ums lunfile, " <<  strerror(errno);
            close(fd);
            return -1;
        }
    }
    close(fd);

    disk->mShared = true;
    vm->mUmsSharingCount += 1;
    if (vm->mUmsSharingCount == 1) {
        FILE* fp;
        int ratio = -1;
        if ((fp = fopen(DIRTY_RATIO_PATH, "r+"))) {
            char line[16];
            if (fgets(line, sizeof(line), fp) && sscanf(line, "%d", &ratio)) {
                vm->mSavedDirtyRatio = ratio;
                fprintf(fp, "%d\n", vm->mUmsDirtyRatio);
                LOG(DEBUG) << getId() << " backup dirty_ratio as " << vm->mSavedDirtyRatio;
                LOG(DEBUG) << getId() << " set dirty_ratio to " << vm->mUmsDirtyRatio;
            } else {
                LOG(ERROR) << getId() << " failed to read dirty_ratio, " <<  strerror(errno);
            }
            fclose(fp);
        } else {
            LOG(ERROR) << getId() << " failed to open, " << DIRTY_RATIO_PATH << ", " <<  strerror(errno);
        }
    }
    return OK;
}

status_t PublicVolume::doUnshare() {
    VolumeManager *vm =VolumeManager::Instance();
    int fd;
    if ((fd = open(mUmsFilePath, O_WRONLY)) < 0) {
       LOG(ERROR) << getId() << " unable to open ums lunfile, " << mUmsFilePath << ", " <<  strerror(errno);
       return -1;
    }

    auto disk = vm->findDisk(getDiskId());
    if (disk == nullptr) {
        LOG(ERROR) << getId() << " cannot find disk, " << getDiskId();
        errno = EINVAL;
        close(fd);
        return -1;
    }
    if (!disk->mShared) {
        LOG(ERROR) << getId() << " disk, " << getDiskId() << ", is already un-shared";
        close(fd);
        return OK;
    }

    char ch = 0;
    #define UNSHARE_RETRIES 300
    #define UNSHARE_RETRY_GAP_MS 200
    for(int i=0; i < UNSHARE_RETRIES; i++) {
        if (write(fd, &ch, 1) < 0) {
            if (i%10 == 0) {
               LOG(ERROR) << getId() << " unable to write to ums lunfile, " <<  strerror(errno) << ", retry=" << i;
            }

            if ( i >= (UNSHARE_RETRIES -1)) {
               LOG(ERROR) << getId() << " Retry Fail: unable to write to ums lunfile, " << strerror(errno);
               close(fd);
               return -1;
            }
            usleep(UNSHARE_RETRY_GAP_MS*1000);
        }
        else {
           break;
        }
    }
    close(fd);

    disk->mShared = false;
    vm->mUmsSharingCount -= 1;
    if (vm->mUmsSharingCount == 0 && vm->mSavedDirtyRatio != -1) {
        FILE* fp;
        if ((fp = fopen(DIRTY_RATIO_PATH, "r+"))) {
            fprintf(fp, "%d\n", vm->mSavedDirtyRatio);
            LOG(DEBUG) << getId() << " restore dirty_ratio to " << vm->mSavedDirtyRatio;
            fclose(fp);
        } else {
           LOG(ERROR) << getId() << " failed to open, " << DIRTY_RATIO_PATH << ", " <<  strerror(errno);
        }
        vm->mSavedDirtyRatio = -1;
    }
    return OK;
}

}  // namespace vold
}  // namespace android

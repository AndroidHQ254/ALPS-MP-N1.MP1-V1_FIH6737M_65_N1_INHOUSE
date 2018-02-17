/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include "Disk.h"
#include "VolumeManager.h"
#include "CommandListener.h"
#include "CryptCommandListener.h"
#include "NetlinkManager.h"
#include "cryptfs.h"
#include "sehandle.h"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <cutils/klog.h>
#include <cutils/properties.h>
#include <cutils/sockets.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <getopt.h>
#include <fcntl.h>
#include <dirent.h>
#include <fs_mgr.h>
#ifdef MTK_FAT_ON_NAND
#include "fat_on_nand.h"
#include <linux/loop.h>
#endif
#include "VoldUtil.h"
#include <sys/mount.h>
#include <log/log.h>

static int process_config(VolumeManager *vm);
static void coldboot(const char *path);
static void parse_args(int argc, char** argv);

struct fstab *fstab;

struct selabel_handle *sehandle;

using android::base::StringPrintf;


#ifndef MTK_EMULATOR_SUPPORT
  #include "libnvram.h"
#endif
#include "CFG_OMADMUSB_File.h"
#include <semaphore.h>

#define MT_NORMAL_BOOT 0
#define MT_META_BOOT 1
#define MT_FACTORY_BOOT 4
#define MT_FACTORY_ATE_BOOT 6


extern int iFileOMADMUSBLID;
int coldboot_sent_uevent_count=0;
static bool coldboot_sent_uevent_count_only = true;
sem_t coldboot_sem;

int main(int argc, char** argv) {
    setenv("ANDROID_LOG_TAGS", "*:v", 1);
    android::base::InitLogging(argv, android::base::LogdLogger(android::base::SYSTEM));

    LOG(INFO) << "Vold 3.0 (the awakening) firing up";

    char crypto_state[PROPERTY_VALUE_MAX];
    property_get("ro.crypto.state", crypto_state, "");
    if((!strcmp(crypto_state, "unencrypted"))||(!strcmp(crypto_state, "unsupported"))) {
       disable_usb_by_dm();
    }

    LOG(VERBOSE) << "Detected support for:"
            << (android::vold::IsFilesystemSupported("ext4") ? " ext4" : "")
            << (android::vold::IsFilesystemSupported("f2fs") ? " f2fs" : "")
            << (android::vold::IsFilesystemSupported("vfat") ? " vfat" : "")
            << (android::vold::IsFilesystemSupported("exfat") ? " exfat" : "");

    VolumeManager *vm;
    CommandListener *cl;
    CryptCommandListener *ccl;
    NetlinkManager *nm;

    parse_args(argc, argv);

    sehandle = selinux_android_file_context_handle();
    if (sehandle) {
        selinux_android_set_sehandle(sehandle);
    }

    // Quickly throw a CLOEXEC on the socket we just inherited from init
    fcntl(android_get_control_socket("vold"), F_SETFD, FD_CLOEXEC);
    fcntl(android_get_control_socket("cryptd"), F_SETFD, FD_CLOEXEC);

    mkdir("/dev/block/vold", 0755);

    /* For when cryptfs checks and mounts an encrypted filesystem */
    klog_set_level(6);

    /* Create our singleton managers */
    if (!(vm = VolumeManager::Instance())) {
        LOG(ERROR) << "Unable to create VolumeManager";
        exit(1);
    }

    if (!(nm = NetlinkManager::Instance())) {
        LOG(ERROR) << "Unable to create NetlinkManager";
        exit(1);
    }

    if (property_get_bool("vold.debug", false)) {
        vm->setDebug(true);
    }

    cl = new CommandListener();
    ccl = new CryptCommandListener();
    vm->setBroadcaster((SocketListener *) cl);
    nm->setBroadcaster((SocketListener *) cl);

    if (vm->start()) {
        PLOG(ERROR) << "Unable to start VolumeManager";
        exit(1);
    }

    if (process_config(vm)) {
        PLOG(ERROR) << "Error reading configuration... continuing anyways";
    }

    if (nm->start()) {
        PLOG(ERROR) << "Unable to start NetlinkManager";
        exit(1);
    }

    if (sem_init(&coldboot_sem, 0, 0) == -1) {
        SLOGE("Unable to sem_init (%s)", strerror(errno));
        exit(1);
    }
    coldboot_sent_uevent_count_only = true;
    coldboot("/sys/block");
    coldboot_sent_uevent_count_only = false;
    coldboot("/sys/block");
//    coldboot("/sys/class/switch");

    SLOGI("Coldboot: try to wait for uevents, timeout 5s");
    struct timespec ts;
    ts.tv_sec=time(NULL)+5;
    ts.tv_nsec=0;
    if (sem_timedwait(&coldboot_sem, &ts) == -1) {
       SLOGE("Coldboot: fail for sem_timedwait(%s)", strerror(errno));
    }
    else {
       SLOGI("Coldboot: all uevent has handled");
    }
    get_is_nvram_in_data();

    /*
     * Now that we're up, we can respond to commands
     */
    if (cl->startListener()) {
        PLOG(ERROR) << "Unable to start CommandListener";
        exit(1);
    }

    if (ccl->startListener()) {
        PLOG(ERROR) << "Unable to start CryptCommandListener";
        exit(1);
    }


    if(!strcmp(crypto_state, "encrypted")) {
       char decrypt_state[PROPERTY_VALUE_MAX];
       property_get("vold.decrypt", decrypt_state, "");
       char crypto_type[PROPERTY_VALUE_MAX];
       property_get("ro.crypto.type", crypto_type, "");

       if (!strcmp(crypto_type, "block")) {
           while (strcmp(decrypt_state, "trigger_restart_framework")) {
              sleep(5);
              property_get("vold.decrypt", decrypt_state, "");
           }
       }
    }
    else {
       // wait for persist property loaded
       sleep(2);
    }


    char no_af[PROPERTY_VALUE_MAX];
    property_get("persist.chiptest.enable", no_af, "0");
    if((get_boot_mode() == MT_NORMAL_BOOT && !strcmp(no_af, "1")) ||
        get_boot_mode() == MT_META_BOOT || get_boot_mode() == MT_FACTORY_BOOT ||
        get_boot_mode() == MT_FACTORY_ATE_BOOT )
    {
        vm->mountallVolumes();
    }

    int fd;
    unsigned long nr_sec = 0;
    if ((fd = open("/dev/block/mmcblk0", O_RDWR | O_CLOEXEC)) != -1) {
        get_blkdev_size(fd, &nr_sec);
        if (nr_sec != 0) {
              char emmc_size[PROPERTY_VALUE_MAX];
              sprintf(emmc_size, "%lu", nr_sec*512);
              LOG(INFO) << "emmc size = " << emmc_size;
              property_set("vold.emmc_size", emmc_size);
        }
        close(fd);
    }

    // Eventually we'll become the monitoring thread
    while(1) {
        sleep(1000);
    }

    LOG(ERROR) << "Vold exiting";
    exit(0);
}

static void parse_args(int argc, char** argv) {
    static struct option opts[] = {
        {"blkid_context", required_argument, 0, 'b' },
        {"blkid_untrusted_context", required_argument, 0, 'B' },
        {"fsck_context", required_argument, 0, 'f' },
        {"fsck_untrusted_context", required_argument, 0, 'F' },
    };

    int c;
    while ((c = getopt_long(argc, argv, "", opts, nullptr)) != -1) {
        switch (c) {
        case 'b': android::vold::sBlkidContext = optarg; break;
        case 'B': android::vold::sBlkidUntrustedContext = optarg; break;
        case 'f': android::vold::sFsckContext = optarg; break;
        case 'F': android::vold::sFsckUntrustedContext = optarg; break;
        }
    }

    CHECK(android::vold::sBlkidContext != nullptr);
    CHECK(android::vold::sBlkidUntrustedContext != nullptr);
    CHECK(android::vold::sFsckContext != nullptr);
    CHECK(android::vold::sFsckUntrustedContext != nullptr);
}

static void do_coldboot(DIR *d, int lvl) {
    struct dirent *de;
    int dfd, fd;

    dfd = dirfd(d);

    fd = openat(dfd, "uevent", O_WRONLY | O_CLOEXEC);
    if(fd >= 0) {
        if (coldboot_sent_uevent_count_only) {
           coldboot_sent_uevent_count++;
        }
        else {
           write(fd, "add\n", 4);
        }
        close(fd);
    }

    while((de = readdir(d))) {
        DIR *d2;

        if (de->d_name[0] == '.')
            continue;

        if (de->d_type != DT_DIR && lvl > 0)
            continue;

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if(fd < 0)
            continue;

        d2 = fdopendir(fd);
        if(d2 == 0)
            close(fd);
        else {
            do_coldboot(d2, lvl + 1);
            closedir(d2);
        }
    }
}

static void coldboot(const char *path) {
    DIR *d = opendir(path);
    if(d) {
        do_coldboot(d, 0);
        closedir(d);
    }
}

static int process_config(VolumeManager *vm) {
    std::string path(android::vold::DefaultFstabPath());
    fstab = fs_mgr_read_fstab(path.c_str());
    if (!fstab) {
        PLOG(ERROR) << "Failed to open default fstab " << path;
        return -1;
    }

    /* Loop through entries looking for ones that vold manages */
    bool has_adoptable = false;
    bool has_support_external_sd = false;
    for (int i = 0; i < fstab->num_entries; i++) {
        if (fs_mgr_is_voldmanaged(&fstab->recs[i])) {
            if (fs_mgr_is_nonremovable(&fstab->recs[i])) {
                LOG(WARNING) << "nonremovable no longer supported; ignoring volume";
                continue;
            }

            std::string sysPattern(fstab->recs[i].blk_device);
            std::string nickname(fstab->recs[i].label);
            int flags = 0;

            if (fs_mgr_is_encryptable(&fstab->recs[i])) {
                flags |= android::vold::Disk::Flags::kAdoptable;
                has_adoptable = true;
            }
            if (fs_mgr_is_noemulatedsd(&fstab->recs[i])
                    || property_get_bool("vold.debug.default_primary", false)) {
                flags |= android::vold::Disk::Flags::kDefaultPrimary;
            }
            if (!has_support_external_sd &&
                (strstr(fstab->recs[i].blk_device, "msdc1") ||
                 strstr(fstab->recs[i].blk_device, "msdc.1") ||
                 strstr(fstab->recs[i].blk_device, "MSDC1"))) {
                has_support_external_sd = true;
            }

            vm->addDiskSource(std::shared_ptr<VolumeManager::DiskSource>(
                    new VolumeManager::DiskSource(sysPattern, nickname, flags)));
#ifdef MTK_FAT_ON_NAND
                if (!strncmp(fstab->recs[i].blk_device, "/devices/virtual/block/loop", 27)) {
                    int mLoopDeviceIdx = atoi((fstab->recs[i].blk_device+27));
                    /* Bind loop device */
                    char devicepath[100];
                    char fatimgfilepath[200];
                    int fd, ffd, mode;
                    struct loop_info loopinfo;
                    const char *fat_mnt;
                    const char *fat_filename;

                    fat_mnt = FAT_PARTITION_MOUNT_POINT;
                    fat_filename = FAT_IMG_NAME;
                    snprintf(devicepath, sizeof(devicepath), "/dev/block/loop%d", mLoopDeviceIdx);
                    snprintf(fatimgfilepath, sizeof(fatimgfilepath), "%s/%s", fat_mnt, fat_filename);
                    mode = O_RDWR;
                    if ((ffd = open(fatimgfilepath, mode)) < 0) {
                        if (ffd < 0) {
                            SLOGE("[FON]Fail to open %s %s(%d)", fatimgfilepath, strerror(errno), errno);
                        }
                    }
                    if ((fd = open(devicepath, mode)) < 0) {
                        SLOGE("[FON]Fail to open %s %s(%d)", devicepath, strerror(errno), errno);
                    }
                    /* Determine loop device is available or not */
                    if (ioctl(fd, LOOP_GET_STATUS, &loopinfo) < 0 && errno == ENXIO) {
                        if (ioctl(fd, LOOP_SET_FD, ffd) < 0) {
                        SLOGE("[FON]Fail to ioctl: LOOP_SET_FD %s(%d)", strerror(errno), errno);
                        }
                    }
                    else {
                        SLOGE("[FON]Fail to bind FAT image %s to %s.Loop device is busy!", fatimgfilepath, devicepath);
                    }
                    close(fd);
                    close(ffd);
                }
#endif
        }
    }
    property_set("vold.has_adoptable", has_adoptable ? "1" : "0");
    property_set("vold.support_external_sd", has_support_external_sd ? "1" : "0");
    return 0;
}

bool is_nvram_in_data = true;
void get_is_nvram_in_data() {
  if (fs_mgr_get_entry_for_mount_point(fstab, "/nvdata")) {
         LOG(INFO) << "'nvdata' partition exists.";
         is_nvram_in_data = false;
  }
  else {
      LOG(INFO) << "'nvdata' partition is not found.";
  }
}

char* get_usb_cmode_path(){
     char propbuf[PROPERTY_VALUE_MAX];
     property_get("ro.hardware", propbuf, "");

     if (strstr(propbuf, "mt6595") || strstr(propbuf, "mt6795")) {
       return (char*)"/sys/class/udc/musb-hdrc/device/cmode";
     }else if (strstr(propbuf, "mt6755") || strstr(propbuf, "mt6797")) {
       return (char*)"/sys/bus/platform/devices/musb-hdrc/cmode";
     }else {
       return (char*)"/sys/devices/platform/mt_usb/cmode";
     }
}

int get_boot_mode(void)
{
  int fd;
  ssize_t s;
  char boot_mode[4] = {'0'};
  char boot_mode_sys_path[] = "/sys/class/BOOT/BOOT/boot/boot_mode";

  fd = open(boot_mode_sys_path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
  if (fd < 0)
  {
    SLOGE("fail to open %s: %s", boot_mode_sys_path, strerror(errno));
    return 0;
  }

  s = read(fd, boot_mode, sizeof(boot_mode) - 1);
  close(fd);

  if(s <= 0)
  {
    SLOGE("could not read boot mode sys file: %s", strerror(errno));
    return 0;
  }

  boot_mode[s] = '\0';
  return atoi(boot_mode);
}

int is_meta_boot(void)
{
  return (get_boot_mode()==1);
}

void create_link_for_mtklogger(const char *ext_sd_path)
{
    static char link_path[255];
    if (ext_sd_path == NULL) {
        ext_sd_path = link_path;
    }
    else {
        strcpy(link_path, ext_sd_path);
    }

    SLOGD("%s(ext_sd_path = %s)", __FUNCTION__, ext_sd_path);
    int ret = -1, round = 3 ;
    while(1) {
        ret = symlink(ext_sd_path, EXT_SDCARD_TOOL);
        round-- ;

        if(ret) {
            if((round > 0) && (errno == EEXIST)) {
                SLOGE("The link already exists!");
                SLOGE("Try again! round : %d", round);
                unlink(EXT_SDCARD_TOOL) ;
            }
            else {
                SLOGE("Create symlink failed.");
                break ;
            }
        }
        else {
            SLOGD("The link is created successfully!");
            break ;
        }
    }
    return;
}

void invoke_coldboot() {
   coldboot("/sys/block");
}


#ifdef MTK_EMULATOR_SUPPORT
void disable_usb_by_dm(){};
int power_control_for_external_sd(int state){return 0;};
#else
#include "../libfile_op/libfile_op.h"
int NvramAccessForOMADM(OMADMUSB_CFG_Struct *st, bool isRead) {
    if (!st) return -1;

    int file_lid = iFileOMADMUSBLID;
    F_ID nvram_fid;
    int rec_size;
    int rec_num;
    int Ret = 0;

    nvram_fid = NVM_GetFileDesc(file_lid, &rec_size, &rec_num, isRead);
    if (nvram_fid.iFileDesc < 0) {
        SLOGE("Unable to open NVRAM file!");
        return -1;
    }

    if (isRead) {
        Ret = read(nvram_fid.iFileDesc, st, rec_size*rec_num);//read NVRAM
    }
    else {
        Ret = write(nvram_fid.iFileDesc, st, rec_size*rec_num);//write NVRAM
    }

    if (Ret < 0) {
        SLOGE("access NVRAM error!");
        return -1;
    }

        if (!isRead){
            FileOp_BackupToBinRegionForDM();
        }

    NVM_CloseFileDesc(nvram_fid);
    SLOGD("read st  nvram_fd=%d   Ret=%d  usb=%d  adb=%d  rndis=%d  rec_size=%d  rec_num=%d", nvram_fid.iFileDesc, Ret, st->iUsb, st->iAdb, st->iRndis, rec_size, rec_num);

    return 0;
}

#define MAX_NVRAM_RESTRORE_READY_RETRY_NUM  (20)
static void *do_disable_usb_by_dm(void *ignored){
    int fd = 0;
    char value[20];
    int count = 0;
    int Ret = 0;
    int rec_size;
    int rec_num;
    int file_lid = iFileOMADMUSBLID;
    OMADMUSB_CFG_Struct mStGet;

    int nvram_restore_ready_retry=0;
    char nvram_init_val[PROPERTY_VALUE_MAX] = {0};
    memset(&mStGet, 0, sizeof(OMADMUSB_CFG_Struct));
    SLOGD("Check whether nvram restore ready!\n");
    while(nvram_restore_ready_retry < MAX_NVRAM_RESTRORE_READY_RETRY_NUM){
        nvram_restore_ready_retry++;
        property_get("service.nvram_init", nvram_init_val, "");
        if(strcmp(nvram_init_val, "Ready") == 0){
            SLOGD("nvram restore ready!\n");
            break;
        }else{
            usleep(500*1000);
        }
    }

    if(nvram_restore_ready_retry >= MAX_NVRAM_RESTRORE_READY_RETRY_NUM){
        SLOGD("Get nvram restore ready fail!\n");
    }

    Ret = NvramAccessForOMADM(&mStGet, true);
    SLOGD("OMADM NVRAM read  Ret=%d, IsEnable=%d, Usb=%d, Adb=%d, Rndis=%d", Ret, mStGet.iIsEnable, mStGet.iUsb, mStGet.iAdb, mStGet.iRndis);
    if (Ret < 0) {
        SLOGE("vold main read NVRAM failed!");
    } else {
        if (1 == mStGet.iIsEnable) {//usb enable
            //do nothing
        } else {//usb disable
            char* usb_cmode_path = get_usb_cmode_path();
            if ((fd = open(usb_cmode_path, O_WRONLY)) < 0) {
                SLOGE("Unable to open %s", usb_cmode_path);
                return (void *)(-1);
            }

            count = snprintf(value, sizeof(value), "%d\n", mStGet.iIsEnable);
            Ret = write(fd, value, count);
            close(fd);
            if (Ret < 0) {
                SLOGE("Unable to write %s", usb_cmode_path);
            }
        }
    }
    return (void *)(uintptr_t)Ret;
}

void disable_usb_by_dm(){
    pthread_t t;
    int ret;
    ret = pthread_create(&t, NULL, do_disable_usb_by_dm, NULL);
    if (ret) {
       SLOGE("Cannot create thread to do disable_usb_by_dm()");
       return;
    }
}

#define MSDC_DEVICE_PATH "/dev/misc-sd"

int power_control_for_external_sd(int state) {
    int inode = 0;
    int result = 0;
    LOG(WARNING) << "power_control_for_external_sd, state=" << state;

    inode = open(MSDC_DEVICE_PATH, O_RDONLY);
    if (inode < 0) {
        LOG(WARNING) << "open device error!";
        return -1;
    }

    if (state == MSDC_REINIT_SDCARD)
    {
        result = ioctl(inode, MSDC_REINIT_SDCARD, NULL);
        if(result){
            LOG(WARNING) << "ioctl error!";
            close(inode);
            return result;
        }
        else
            LOG(WARNING) << "power_control_for_external_sd reinitExternalSD success!";
        sleep(1);

    } else if (state == MSDC_SD_POWER_ON) {
        result = ioctl(inode, MSDC_SD_POWER_ON, NULL);
        if (result) {
            LOG(WARNING) << "ioctl error!";
            close(inode);
            return result;
        }
        else
            LOG(WARNING) << "MSDC_SD_POWER_ON success!";

    } else if (state == MSDC_SD_POWER_OFF) {
        result = ioctl(inode, MSDC_SD_POWER_OFF, NULL);
        if(result){
            LOG(WARNING) << "ioctl error!";
            close(inode);
            return result;
            }
        else
        LOG(WARNING) << "MSDC_SD_POWER_OFF success!";
    }



    close(inode);

    return result;
}



#endif

bool isMountpointMounted(const char *mp)
{
    FILE *fp = setmntent("/proc/mounts", "r");
    if (fp == NULL) {
        SLOGE("Error opening /proc/mounts (%s)", strerror(errno));
        return false;
    }

    bool found_mp = false;
    mntent* mentry;
    while ((mentry = getmntent(fp)) != NULL) {
        if (strcmp(mentry->mnt_dir, mp) == 0) {
            found_mp = true;
            break;
        }
    }
    endmntent(fp);
    return found_mp;
}

#include <android-base/file.h>
int write_file_bootprof(const char* content) {
    const char* path = "/proc/bootprof";
    int fd = TEMP_FAILURE_RETRY(open(path, O_WRONLY|O_CREAT|O_NOFOLLOW|O_CLOEXEC, 0600));
    if (fd == -1) {
        SLOGE("%s: Unable to open '%s': %s\n", __FUNCTION__, path, strerror(errno));
        return -1;
    }
    int result = android::base::WriteStringToFd(content, fd) ? 0 : -1;
    if (result == -1) {
        SLOGE("%s: Unable to write to '%s': %s\n", __FUNCTION__, path, strerror(errno));
    }
    close(fd);
    return result;
}

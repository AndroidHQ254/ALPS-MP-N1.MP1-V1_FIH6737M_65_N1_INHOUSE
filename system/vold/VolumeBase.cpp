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

#include "Utils.h"
#include "VolumeBase.h"
#include "VolumeManager.h"
#include "ResponseCode.h"

#include <android-base/stringprintf.h>
#include <android-base/logging.h>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>

using android::base::StringPrintf;

#define DEBUG 1

namespace android {
namespace vold {

VolumeBase::VolumeBase(Type type) :
        mType(type), mMountFlags(0), mMountUserId(-1), mCreated(false), mState(
                State::kUnmounted), mSilent(false) {
        mStorageType=StorageType::kEmulated;
}

VolumeBase::~VolumeBase() {
    CHECK(!mCreated);
}

void VolumeBase::setState(State state) {
    mState = state;
    LOG(WARNING) << getId() << " setState: " << StringPrintf("%d", mState);
    notifyEvent(ResponseCode::VolumeStateChanged, StringPrintf("%d", mState));
}

status_t VolumeBase::setDiskId(const std::string& diskId) {
    if (mCreated) {
        LOG(WARNING) << getId() << " diskId change requires destroyed";
        return -EBUSY;
    }

    mDiskId = diskId;
    return OK;
}

status_t VolumeBase::setPartGuid(const std::string& partGuid) {
    if (mCreated) {
        LOG(WARNING) << getId() << " partGuid change requires destroyed";
        return -EBUSY;
    }

    mPartGuid = partGuid;
    return OK;
}

status_t VolumeBase::setMountFlags(int mountFlags) {
    if ((mState != State::kUnmounted) && (mState != State::kUnmountable)) {
        LOG(WARNING) << getId() << " flags change requires state unmounted or unmountable";
        return -EBUSY;
    }

    mMountFlags = mountFlags;
    return OK;
}

status_t VolumeBase::setMountUserId(userid_t mountUserId) {
    if ((mState != State::kUnmounted) && (mState != State::kUnmountable)) {
        LOG(WARNING) << getId() << " user change requires state unmounted or unmountable";
        return -EBUSY;
    }

    mMountUserId = mountUserId;
    return OK;
}

status_t VolumeBase::setSilent(bool silent) {
    if (mCreated) {
        LOG(WARNING) << getId() << " silence change requires destroyed";
        return -EBUSY;
    }

    mSilent = silent;
    return OK;
}

status_t VolumeBase::setId(const std::string& id) {
    if (mCreated) {
        LOG(WARNING) << getId() << " id change requires not created";
        return -EBUSY;
    }

    mId = id;
    return OK;
}

status_t VolumeBase::setPath(const std::string& path) {
    if (mState != State::kChecking) {
        LOG(WARNING) << getId() << " path change requires state checking";
        return -EBUSY;
    }

    mPath = path;
    notifyEvent(ResponseCode::VolumePathChanged, mPath);
    return OK;
}

status_t VolumeBase::setInternalPath(const std::string& internalPath) {
    if (mState != State::kChecking) {
        LOG(WARNING) << getId() << " internal path change requires state checking";
        return -EBUSY;
    }

    mInternalPath = internalPath;
    notifyEvent(ResponseCode::VolumeInternalPathChanged, mInternalPath);
    return OK;
}

void VolumeBase::notifyEvent(int event) {
    if (mSilent) return;
    VolumeManager::Instance()->getBroadcaster()->sendBroadcast(event,
            getId().c_str(), false);
}

void VolumeBase::notifyEvent(int event, const std::string& value) {
    if (mSilent) return;
    VolumeManager::Instance()->getBroadcaster()->sendBroadcast(event,
            StringPrintf("%s %s", getId().c_str(), value.c_str()).c_str(), false);
}

void VolumeBase::addVolume(const std::shared_ptr<VolumeBase>& volume) {
    mVolumes.push_back(volume);
}

void VolumeBase::removeVolume(const std::shared_ptr<VolumeBase>& volume) {
    mVolumes.remove(volume);
}

std::shared_ptr<VolumeBase> VolumeBase::findVolume(const std::string& id) {
    for (auto vol : mVolumes) {
        if (vol->getId() == id) {
            return vol;
        }
    }
    return nullptr;
}

status_t VolumeBase::create() {
    CHECK(!mCreated);

    mCreated = true;
    status_t res = doCreate();
    setState(State::kUnmounted);
    notifyEvent(ResponseCode::VolumeCreated,
            StringPrintf("%d \"%s\" \"%s\"", mType, mDiskId.c_str(), mPartGuid.c_str()));

    return res;
}

status_t VolumeBase::doCreate() {
    return OK;
}

status_t VolumeBase::destroy() {
    CHECK(mCreated);

    if (mState == State::kMounted) {
        unmount();
        setState(State::kBadRemoval);
    } else if (mState == State::kShared ){
        unshare();
        setState(State::kRemoved);
    } else {
        setState(State::kRemoved);
    }

    notifyEvent(ResponseCode::VolumeDestroyed);
    status_t res = doDestroy();
    mCreated = false;
    return res;
}

status_t VolumeBase::doDestroy() {
    return OK;
}

#include "VolumeManager.h"
status_t VolumeBase::mount() {
    if ((mState != State::kUnmounted) && (mState != State::kUnmountable)) {
        LOG(WARNING) << getId() << " mount requires state unmounted or unmountable";
        return -EBUSY;
    }

    setState(State::kChecking);

    status_t res = OK;
    LOG(WARNING) << getId() << " VolumeManager::Instance()->mSkipUmountForResetCommand: " << VolumeManager::Instance()->mSkipUmountForResetCommand;
    if (isMountpointMounted(getPath().c_str())) {
        LOG(WARNING) << getId() << " mPath: " << mPath << " is mounted. skip doMount()";
        setInternalPath(mInternalPath);
        setPath(mPath);
    }
    else {
        res = doMount();
    }

    if (res == OK) {
        setState(State::kMounted);
        createRemoveStorageTypeSettings(true);
    } else {
        setState(State::kUnmountable);
        if (mStorageType == StorageType::kExternalSD) {
            power_control_for_external_sd(MSDC_SD_POWER_OFF);
        }
    }

    return res;
}

status_t VolumeBase::unmount() {
    if (mState != State::kMounted) {
        LOG(WARNING) << getId() << " unmount requires state mounted";
        return -EBUSY;
    }

    setState(State::kEjecting);
    for (auto vol : mVolumes) {
        if (vol->destroy()) {
            LOG(WARNING) << getId() << " failed to destroy " << vol->getId()
                    << " stacked above";
        }
    }
    mVolumes.clear();

    status_t res = OK;
    if (!VolumeManager::Instance()->mSkipUmountForResetCommand) {
      res = doUnmount();
    }
    setState(State::kUnmounted);
    createRemoveStorageTypeSettings(false);
    return res;
}

status_t VolumeBase::format(const std::string& fsType) {
    if (mState == State::kMounted) {
        unmount();
    }

    if ((mState != State::kUnmounted) && (mState != State::kUnmountable)) {
        LOG(WARNING) << getId() << " format requires state unmounted or unmountable";
        return -EBUSY;
    }

    setState(State::kFormatting);
    status_t res = doFormat(fsType);
    setState(State::kUnmounted);
    if (res != OK) {
        if (mStorageType == StorageType::kExternalSD) {
            power_control_for_external_sd(MSDC_SD_POWER_OFF);
        }
    }
    return res;
}

status_t VolumeBase::doFormat(const std::string& fsType) {
    return -ENOTSUP;
}

#include <cutils/properties.h>
#include <private/android_filesystem_config.h>
#include <cutils/fs.h>

status_t VolumeBase::share() {
    if ((mState != State::kUnmounted) && (mState != State::kUnmountable)) {
        LOG(WARNING) << getId() << " share requires state unmounted or unmountable";
        return -EBUSY;
    }

    if (mType != Type::kPublic) {
       // only portable storage can share
       LOG(WARNING) << getId() << " share requires public volume";
       errno = EINVAL;
       return -1;
    }

    status_t res = doShare();
    if (!res) {
      setState(State::kShared);
    }
    return res;
}

status_t VolumeBase::unshare() {
    if ((mState != State::kShared)) {
        LOG(WARNING) << getId() << " unshare requires state shared";
        return -EINVAL;
    }

    if (mType != Type::kPublic) {
       // only portable storage can share
       LOG(WARNING) << getId() << " unshare requires public volume";
       errno = EINVAL;
       return -1;
    }

    status_t res = doUnshare();
    if (!res) {
      setState(State::kUnmounted);
    }
    return res;
}

void VolumeBase::listVolumes(VolumeBase::Type type, std::list<std::string>& list) {
    for (auto vol : mVolumes) {
        if (vol->getType() == type) {
            list.push_back(vol->getId());
        }
    }
}

void VolumeBase::setStorageType(const StorageType type) {
    mStorageType = type;
}

void VolumeBase::createRemoveStorageTypeSettings(bool is_create) {
    userid_t userId = 0; /* create the link only for owner */
    char stoarge_path_prop[PROPERTY_VALUE_MAX];
    char stoarge_type[PROPERTY_VALUE_MAX];

    if(mType == Type::kPrivate) {
        return;
    }

    switch(mStorageType){
        case StorageType::kEmulated: {
             sprintf(stoarge_type, "internal_storage");
             break;
        }
        case StorageType::kExternalSD: {
             sprintf(stoarge_type, "external_sd");
             break;
        }
        case StorageType::kPhoneStorage: {
             sprintf(stoarge_type, "phone_storage");
             break;
        }
    }

    std::string source(getPath());
    if (getType() == Type::kEmulated) {
        source = StringPrintf("%s/%d", source.c_str(), userId);
        fs_prepare_dir(source.c_str(), 0755, AID_ROOT, AID_ROOT);
    }


    std::string target(StringPrintf("/mnt/m_%s", stoarge_type));
    if (TEMP_FAILURE_RETRY(unlink(target.c_str()))) {
        if (errno != ENOENT) {
            LOG(WARNING) << "Failed to unlink " << target.c_str() << ": " << strerror(errno);
        }
    }

    if(mStorageType == StorageType::kExternalSD) {
        if (TEMP_FAILURE_RETRY(unlink("/storage/sdcard1"))) {
              if (errno != ENOENT) {
                  LOG(WARNING) << "Failed to unlink /storage/sdcard1: " << strerror(errno);
              }
        }
    }

    if (is_create) {
        LOG(INFO) << "Linking " << source << " to " << target;
        if (TEMP_FAILURE_RETRY(symlink(source.c_str(), target.c_str()))) {
            LOG(WARNING) << "Failed to link " << source.c_str() << " to " << target.c_str() << ": " << strerror(errno);
        }

        if(mStorageType == StorageType::kExternalSD) {
         create_link_for_mtklogger(source.c_str());
         if (!(mMountFlags & kPrimary)) {
             if (TEMP_FAILURE_RETRY(symlink(source.c_str(), "/storage/sdcard1"))) {
                     LOG(WARNING) << "Failed to link " << source.c_str() << " to /storage/sdcard1: " << strerror(errno);
                 }
             }
         }
    }

    sprintf(stoarge_path_prop, "vold.path.%s", stoarge_type);
    if(is_create) {
         property_set(stoarge_path_prop, source.c_str());
    }
    else {
         property_set(stoarge_path_prop, "");
    }
}

}  // namespace vold
}  // namespace android

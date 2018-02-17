/*
* Copyright (C) 2014 MediaTek Inc.
* Modification based on code covered by the mentioned copyright
* and/or permission notice(s).
*/
/*
**
** Copyright 2012, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef ANDROID_AUDIO_PULSE_H
#define ANDROID_AUDIO_PULSE_H

#include <system/audio.h>

//#define MTK_LATENCY_DETECT_PULSE

__BEGIN_DECLS

void detectPulse(const int TagNum, const int pulseLevel, const int dump, void* ptr,
                 const size_t desiredFrames, const audio_format_t format,
                 const int channels, const int sampleRate);

__END_DECLS

#endif


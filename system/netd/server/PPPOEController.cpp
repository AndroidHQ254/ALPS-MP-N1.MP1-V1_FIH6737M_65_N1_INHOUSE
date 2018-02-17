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

#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <cutils/properties.h>
#include <netutils/ifc.h>
#include <sys/wait.h>

#define LOG_TAG "PPPOEController"
#include <cutils/log.h>

#include "PPPOEController.h"



PPPOEController::PPPOEController() {

}

PPPOEController::~PPPOEController() {
}


int PPPOEController::stopPPPOE() {

	pid_t pid = 0;
	char pid_str[128] = {0};
	char pid_value[128] = {0};
	char command[92] = {0};
	char iface[128] = {0};

	property_get("net.pppoe.interface", iface, NULL);

	snprintf(pid_str, 128, "pppoe.%s.pid", iface);
	if (property_get(pid_str, pid_value, NULL))
	{
        pid = atol(pid_value);
	}

	if (pid > 0)
	{
		snprintf(command, 92, "kill -9 %d", pid);
		ALOGD("system: %s", command);
		if (-1 == system(command))
		{
			ALOGE("Failed to execute kill system command.");
			return -1;
		}
		return 0;
	}
	else
	{
		ALOGE("ERROR: pid less than 0");
		return -1;
	}
}

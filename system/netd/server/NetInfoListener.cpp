/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "NetInfoListener"
#include <cutils/log.h>

#include "NetInfoListener.h"
#include "ResponseCode.h"

SipInfoHandler::SipInfoHandler(char *ifname,
					   char *server,
					   char* service,
					   char* host,
					   char* port,
					   char* af){
	mIface = strdup(ifname);
    mServer = strdup(server);
    mService = strdup(service);
    mHost = strdup(host);
    mPort = strdup(port);
	mAf = atoi(af);
}

SipInfoHandler::~SipInfoHandler() {
    free(mIface);
    mIface = NULL;
	free(mServer);
    mServer = NULL;
    free(mService);
    mService = NULL;
    free(mHost);
    mHost = NULL;
    free(mPort);
    mPort = NULL;
}

int SipInfoHandler::dumpMember(){
	ALOGI("iface=%s,server=%s,service=%s,host=%s,port=%s",
		mIface, mServer, mService, mHost,mPort);
	return 0;
}

NetInfoListener::NetInfoListener() {
	mSipInfo = new SipInfoCollection();
	//mDhcpv4Pid = 0;
}

NetInfoListener::~NetInfoListener() {
	delete mSipInfo;
}
/*
int NetInfoListener::startDhcpv4(const char* iface) {
    pid_t pid;

    if (mDhcpv4Pid) {
        ALOGE("dhcov4 is already started");
        errno = EBUSY;
        return -1;
    }

   if ((pid = fork()) < 0) {
        ALOGE("fork failed (%s)", strerror(errno));
        return -1;
    }

    if (!pid) {
        if (execl("/system/bin/dhcpcd", "/system/bin/dhcpcd",
                  iface, "-s", "-BK", "-A", (char *) NULL)) {
            ALOGE("execl failed (%s)", strerror(errno));
        }
        ALOGE("Should never get here!");
        return 0;
    } else {
        mDhcpv4Pid = pid;
        ALOGD("dhcpcd pid = %d ", mDhcpv4Pid);
    }
    return 0;

}
*/
// 0         1       2        3        4         5     6     7
//NetInfo setsip ifname server service host port af
int NetInfoListener::setSipInfo(const int argc, char **argv) {
	if(argc < 8){
		ALOGE("setSipInfo invalid argc %d", argc);
		return -1;
	}
	SipInfoHandler* handler = 
		new SipInfoHandler(argv[2],argv[3], argv[4],argv[5],argv[6],argv[7]);

    mSipInfo->push_back(handler);
	handler->dumpMember();
	ALOGI("setSipInfo done");
  	return 0;
}

int NetInfoListener::getSipInfo(const char *iface, 
	const char* service, const char* protocol, SocketClient* cli) {
	
	SipInfoCollection::iterator i;
	
	ALOGI("getSipInfo start, iface = %s, service = %s, protocol = %s", 
		iface, service, protocol);
	if(mSipInfo->size() == 0){
		cli->sendMsg(ResponseCode::NetInfoSipError, "getsip error: no record!", false);
		ALOGI("getsip error: no record!");
		return 0;
	}
    int af;
	if(0 == strcmp("v4",protocol)){
		af = AF_INET;
	} else if(0 == strcmp("v6",protocol)){
		af = AF_INET6;
	} else if(0 == strcmp("v4v6",protocol)){
		af = AF_UNSPEC;
	} else {
		cli->sendMsg(ResponseCode::NetInfoSipError, "getsip error: invalid protocol!", false);
		ALOGI("getsip error: invalid protocol!");
		return 0;
	}
		
    for (i = mSipInfo->begin(); i != mSipInfo->end(); ++i) {
        SipInfoHandler *c = *i;
        if ((0 == strcmp(iface, c->getIface())) &&
			    (0 == strcmp(service, c->getService())) && 
			    (af == AF_UNSPEC || af == c->getAf())) {
			char *msg = NULL;
            asprintf(&msg, "%s %s",c->getHost(), c->getPort());
            cli->sendMsg(ResponseCode::NetInfoSipResult, msg, false);
			ALOGI("getSipInfo msg = %s", msg);
            free(msg);
			return 0;
        }
    }
	cli->sendMsg(ResponseCode::NetInfoSipError, "getsip: no record is found!", false);
	ALOGI("getsip error: no record is found!");
	return 0;
}

int NetInfoListener::clearSipInfo(const char *iface) {
	SipInfoCollection::iterator i;
	
	ALOGI("clearSipInfo start, iface = %s", iface);
	if(mSipInfo->size() == 0){
		ALOGI("clearSip Info error: no record!");
		return 0;
	}
    for (i = mSipInfo->begin(); i != mSipInfo->end();) {
        SipInfoHandler *c = *i;
        if (0 == strcmp(iface, c->getIface())) {
			delete c;
			i = mSipInfo->erase(i);
        }
    }
	{			
		char sip_prop_name[PROPERTY_KEY_MAX];
		snprintf(sip_prop_name, sizeof(sip_prop_name),
				"dhcp.%s.v4sipinfo", iface);
		property_set(sip_prop_name, "");
		ALOGI("clearSip Info, clear property done!");
	}
  return 0;
}

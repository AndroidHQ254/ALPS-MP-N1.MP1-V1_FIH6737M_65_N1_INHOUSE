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
#ifndef _NETINFO_LISTENER_H
#define _NETINFO_LISTENER_H

#include <sysutils/List.h>
#include <sysutils/SocketClient.h>

class SipInfoHandler {
public:
	// Note: All of host, service, and hints may be NULL
	SipInfoHandler(char *ifname,
					   char *server,
					   char* service,
					   char* host,
					   char* port,
					   char* af);
	~SipInfoHandler();
    const char *getIface() { return mIface; }
    const char *getService() { return mService; }
	const char *getHost() { return mHost; }
	const char *getPort() { return mPort; }
	int getAf(){ return mAf; }
	int dumpMember();

private:
	char* mIface;
	char* mServer;
	char* mService; 
	char* mHost;
	char* mPort; 
	int   mAf;
};

typedef android::sysutils::List<SipInfoHandler *> SipInfoCollection;

class NetInfoListener {
public:

    NetInfoListener();
    virtual ~NetInfoListener();
    int setSipInfo(const int argc, char **argv);
    int getSipInfo(const char *iface, const char* service, const char* protocol,SocketClient* cli);
    int clearSipInfo(const char *ifname);

 private:
	 
	 SipInfoCollection *mSipInfo;

};


#endif

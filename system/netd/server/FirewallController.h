/*
* Copyright (C) 2014 MediaTek Inc.
* Modification based on code covered by the mentioned copyright
* and/or permission notice(s).
*/
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

#ifndef _FIREWALL_CONTROLLER_H
#define _FIREWALL_CONTROLLER_H

#include <string>

#include <vector>

#include <set>

#include <utils/RWLock.h>

#include "NetdConstants.h"

enum FirewallRule { DENY, ALLOW };

// WHITELIST means the firewall denies all by default, uids must be explicitly ALLOWed
// BLACKLIST means the firewall allows all by default, uids must be explicitly DENYed

enum FirewallType { WHITELIST, BLACKLIST };

enum ChildChain { NONE, DOZABLE, STANDBY, POWERSAVE, INVALID_CHAIN };

#define PROTOCOL_TCP 6
#define PROTOCOL_UDP 17

///M: Support PPTP
enum FirewallChinaRule { MOBILE, WIFI };

#define PROTOCOL_GRE 47
#define PROTOCOL_ICMP   1

static const char* NSIOT_WHITE_LIST[] = {
        "1.1.1.1",
        "1.2.3.4",
        NULL,
};

/*
 * Simple firewall that drops all packets except those matching explicitly
 * defined ALLOW rules.
 *
 * Methods in this class must be called when holding a write lock on |lock|, and may not call
 * any other controller without explicitly managing that controller's lock. There are currently
 * no such methods.
 */
class FirewallController {
public:
    FirewallController();

    int setupIptablesHooks(void);

    int enableFirewall(FirewallType);
    int disableFirewall(void);
    int isFirewallEnabled(void);

    /* Match traffic going in/out over the given iface. */
    int setInterfaceRule(const char*, FirewallRule);
    /* Match traffic coming-in-to or going-out-from given address. */
    int setEgressSourceRule(const char*, FirewallRule);
    /* Match traffic coming-in-from or going-out-to given address, port, and protocol. */
    int setEgressDestRule(const char*, int, int, FirewallRule);
    /* Match traffic owned by given UID. This is specific to a particular chain. */
    int setUidRule(ChildChain, int, FirewallRule);

    int enableChildChains(ChildChain, bool);

    int replaceUidChain(const char*, bool, const std::vector<int32_t>&);

    static const char* TABLE;

    static const char* LOCAL_INPUT;
    static const char* LOCAL_OUTPUT;
    static const char* LOCAL_FORWARD;

    static const char* LOCAL_DOZABLE;
    static const char* LOCAL_STANDBY;
    static const char* LOCAL_POWERSAVE;

    static const char* ICMPV6_TYPES[];

    ///M: @{
    int setEgressProtoRule(const char* proto, FirewallRule rule);
    int setUdpForwarding(const char* inInterface, const char* extInterface, const char* ipAddr);
    int clearUdpForwarding(const char* inInterface, const char* extInterface);
    int getUsbClientIp(const char* iface);
    int setNsiotFirewall(void);
    int clearNsiotFirewall(void);
    int setVolteNsiotFirewall(const char* iface);
    int clearVolteNsiotFirewall(const char* iface);
    int setUidFwRule(int, FirewallChinaRule, FirewallRule);
    int clearFwChain(const char* chain);
    int setPlmnIfaceRule(const char* channel, int iface_mask, FirewallRule rule);
    ///@}
    int setPkgUidRule(int uid,FirewallRule rule);
    static const char* LOCAL_MANGLE_POSTROUTING;
    // mtk03594, Support enhanced firewall @{
    static const char* FIREWALL;
    static const char* FIREWALL_MOBILE;
    static const char* FIREWALL_WIFI;
    //@}
    // mtk80842, for blocking benchmark app
    static const char* IPT_PATH;
    //for cellular data control
    static bool checkBlackListUsers(int uid);

    android::RWLock lock;


protected:
    friend class FirewallControllerTest;
    std::string makeUidRules(IptablesTarget target, const char *name, bool isWhitelist,
                             const std::vector<int32_t>& uids);
    static int (*execIptables)(IptablesTarget target, ...);
    static int (*execIptablesSilently)(IptablesTarget target, ...);
    static int (*execIptablesRestore)(IptablesTarget target, const std::string& commands);

private:
    FirewallType mFirewallType;
    int attachChain(const char*, const char*);
    int detachChain(const char*, const char*);
    int createChain(const char*, const char*, FirewallType);
    FirewallType getFirewallType(ChildChain);
    int refreshPkgUidList(const char *file_path, int uid, bool add);
    //mtk for cellular data control
    static std::set<int> blacklistUsers;

};

#endif

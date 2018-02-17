/*
* Copyright (C) 2014 MediaTek Inc.
* Modification based on code covered by the mentioned copyright
* and/or permission notice(s).
*/
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define LOG_TAG "Netd"

#include <cutils/log.h>

#include <netutils/ifc.h>
#include <sysutils/NetlinkEvent.h>
#include "CommandListener.h"
#include "NetlinkHandler.h"
#include "NetlinkManager.h"
#include "ResponseCode.h"
#include "SockDiag.h"

static const char *kUpdated = "updated";
static const char *kRemoved = "removed";

NetlinkHandler::NetlinkHandler(NetlinkManager *nm, int listenerSocket,
                               int format) :
                        NetlinkListener(listenerSocket, format) {
    mNm = nm;
}

NetlinkHandler::~NetlinkHandler() {
}

int NetlinkHandler::start() {
    return this->startListener();
}

int NetlinkHandler::stop() {
    return this->stopListener();
}

void NetlinkHandler::onEvent(NetlinkEvent *evt) {
    const char *subsys = evt->getSubsystem();
    if (!subsys) {
        ALOGW("No subsystem found in netlink event");
        return;
    }

    if (!strcmp(subsys, "net")) {
        NetlinkEvent::Action action = evt->getAction();
        const char *iface = evt->findParam("INTERFACE");
        ALOGI("Get netlink event, ifname: %s action: %d", iface, action);

        if (action == NetlinkEvent::Action::kAdd) {

            notifyInterfaceAdded(iface);
        } else if (action == NetlinkEvent::Action::kRemove) {
            notifyInterfaceRemoved(iface);
        } else if (action == NetlinkEvent::Action::kChange) {
            //evt->dump();
            notifyInterfaceChanged("nana", true);
        } else if (action == NetlinkEvent::Action::kLinkUp) {
            notifyInterfaceLinkChanged(iface, true);
        } else if (action == NetlinkEvent::Action::kLinkDown) {
            notifyInterfaceLinkChanged(iface, false);
        } else if (action == NetlinkEvent::Action::kIPv6Enable) {
            notifyInterfaceIPv6Changed(iface, true);
        } else if (action == NetlinkEvent::Action::kIPv6Disable) {
            notifyInterfaceIPv6Changed(iface, false);
        } else if (action == NetlinkEvent::Action::kAddressUpdated ||
                   action == NetlinkEvent::Action::kAddressRemoved) {

            const char *address = evt->findParam("ADDRESS");
            const char *flags = evt->findParam("FLAGS");
            const char *scope = evt->findParam("SCOPE");
            const char *valid = evt->findParam("VALID");
            if (action == NetlinkEvent::Action::kAddressRemoved && iface && address) {
                // Note: if this interface was deleted, iface is "" and we don't notify.
                SockDiag sd;
                if (sd.open()) {
                    char addrstr[INET6_ADDRSTRLEN];
                    strncpy(addrstr, address, sizeof(addrstr));
                    char *slash = strchr(addrstr, '/');
                    if (slash) {
                        *slash = '\0';
                    }

                    int ret = sd.destroySockets(addrstr);
                    if (ret < 0) {
                        ALOGE("Error destroying sockets: %s", strerror(ret));
                    }
                } else {
                    ALOGE("Error opening NETLINK_SOCK_DIAG socket: %s", strerror(errno));
                }
            }
            if (iface && iface[0] && address && flags && scope) {
                notifyAddressChanged(action, address, iface, flags, scope, valid);
            }
        } else if (action == NetlinkEvent::Action::kRdnss) {
            const char *lifetime = evt->findParam("LIFETIME");
            const char *servers = evt->findParam("SERVERS");
            if (lifetime && servers) {
                notifyInterfaceDnsServers(iface, lifetime, servers);
            }
        } else if (action == NetlinkEvent::Action::kRouteUpdated ||
                   action == NetlinkEvent::Action::kRouteRemoved) {
            const char *route = evt->findParam("ROUTE");
            const char *gateway = evt->findParam("GATEWAY");
            const char *iface = evt->findParam("INTERFACE");
            if (route && (gateway || iface)) {
                notifyRouteChange(action, route, gateway, iface);
            }
        } else if (action == NetlinkEvent::Action::kNoRA) {
            const char *iface = evt->findParam("INTERFACE");
            const char *flags = evt->findParam("FLAGS");
                  /*flags = 1 RA init fail,send -1 to fwk, flags = 0 RA refresh Fail send -2 to fwk */
                  int flag = atoi(flags);
                  if(flag == 0) {
                       notifyAddressChanged(NetlinkEvent::Action::kAddressRemoved, "FE80::22/64", iface, "01", "01","-2");
                  } else {
                       notifyAddressChanged(NetlinkEvent::Action::kAddressRemoved, "FE80::22/64", iface, "01", "01","-1");
                }
            //we use addr change event to nofity fw
        //    notifyAddressChanged(NetlinkEvent::Action::kAddressRemoved, "FE80::22/64", iface, "01", "01","-1");
        }

    } else if (!strcmp(subsys, "qlog") || !strcmp(subsys, "xt_quota2")) {
        const char *alertName = evt->findParam("ALERT_NAME");
        const char *iface = evt->findParam("INTERFACE");
        notifyQuotaLimitReached(alertName, iface);

    } else if (!strcmp(subsys, "strict")) {
        const char *uid = evt->findParam("UID");
        const char *hex = evt->findParam("HEX");
        notifyStrictCleartext(uid, hex);

    } else if (!strcmp(subsys, "xt_idletimer")) {
        const char *label = evt->findParam("INTERFACE");
        const char *state = evt->findParam("STATE");
        const char *timestamp = evt->findParam("TIME_NS");
        const char *uid = evt->findParam("UID");
        if (state)
            notifyInterfaceClassActivity(label, !strcmp("active", state),
                                         timestamp, uid);

#if !LOG_NDEBUG
    } else if (strcmp(subsys, "platform") && strcmp(subsys, "backlight")) {
        /* It is not a VSYNC or a backlight event */
        ALOGV("unexpected event from subsystem %s", subsys);
#endif
    }
}

void NetlinkHandler::notify(int code, const char *format, ...) {
    char *msg;
    va_list args;
    va_start(args, format);
    if (vasprintf(&msg, format, args) >= 0) {
        mNm->getBroadcaster()->sendBroadcast(code, msg, false);
        free(msg);
    } else {
        SLOGE("Failed to send notification: vasprintf: %s", strerror(errno));
    }
    va_end(args);
}

void NetlinkHandler::notifyInterfaceAdded(const char *name) {
    notify(ResponseCode::InterfaceChange, "Iface added %s", name);
}

void NetlinkHandler::notifyInterfaceRemoved(const char *name) {
    notify(ResponseCode::InterfaceChange, "Iface removed %s", name);
}

void NetlinkHandler::notifyInterfaceChanged(const char *name, bool isUp) {
    notify(ResponseCode::InterfaceChange,
           "Iface changed %s %s", name, (isUp ? "up" : "down"));
}

void NetlinkHandler::notifyInterfaceLinkChanged(const char *name, bool isUp) {
    notify(ResponseCode::InterfaceChange,
           "Iface linkstate %s %s", name, (isUp ? "up" : "down"));
}

void NetlinkHandler::notifyQuotaLimitReached(const char *name, const char *iface) {
    notify(ResponseCode::BandwidthControl, "limit alert %s %s", name, iface);
}

void NetlinkHandler::notifyInterfaceClassActivity(const char *name,
                                                  bool isActive,
                                                  const char *timestamp,
                                                  const char *uid) {
    if (timestamp == NULL)
        notify(ResponseCode::InterfaceClassActivity,
           "IfaceClass %s %s", isActive ? "active" : "idle", name);
    else if (uid != NULL && isActive)
        notify(ResponseCode::InterfaceClassActivity,
           "IfaceClass active %s %s %s", name, timestamp, uid);
    else
        notify(ResponseCode::InterfaceClassActivity,
           "IfaceClass %s %s %s", isActive ? "active" : "idle", name, timestamp);
}

void NetlinkHandler::notifyAddressChanged(NetlinkEvent::Action action, const char *addr,
                                          const char *iface, const char *flags,
                                          const char *scope, const char *valid) {
    notify(ResponseCode::InterfaceAddressChange,
           "Address %s %s %s %s %s %s",
           (action == NetlinkEvent::Action::kAddressUpdated) ? kUpdated : kRemoved,
           addr, iface, flags, scope, valid);
}

void NetlinkHandler::notifyInterfaceDnsServers(const char *iface,
                                               const char *lifetime,
                                               const char *servers) {
    notify(ResponseCode::InterfaceDnsInfo, "DnsInfo servers %s %s %s",
           iface, lifetime, servers);
}

void NetlinkHandler::notifyRouteChange(NetlinkEvent::Action action, const char *route,
                                       const char *gateway, const char *iface) {
    notify(ResponseCode::RouteChange,
           "Route %s %s%s%s%s%s",
           (action == NetlinkEvent::Action::kRouteUpdated) ? kUpdated : kRemoved,
           route,
           *gateway ? " via " : "",
           gateway,
           *iface ? " dev " : "",
           iface);
}

void NetlinkHandler::notifyStrictCleartext(const char* uid, const char* hex) {
    notify(ResponseCode::StrictCleartext, "%s %s", uid, hex);
}

void NetlinkHandler::notifyInterfaceIPv6Changed(const char *name, bool isUp) {
    notify(ResponseCode::InterfaceChange,
           "Iface IPv6state %s %s", name, (isUp ? "up" : "down"));
}

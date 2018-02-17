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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>

#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include <android-base/stringprintf.h>

#include <cutils/sockets.h>
#include <private/android_filesystem_config.h>
#ifdef MTK_FAT_ON_NAND
#include "fat_on_nand.h"
#include <sys/statfs.h>
#include <cutils/properties.h>

#define RESERVE_SIZE    (1*1024*1024)  //  actually maybe 70KB, but reserve more for safety
#endif

static void usage(char *progname);
static int do_monitor(int sock, int stop_after_cmd);
static int do_cmd(int sock, int argc, char **argv);

static constexpr int kCommandTimeoutMs = 20 * 1000;

int main(int argc, char **argv) {
    int sock;
    int wait_for_socket;
    char *progname;

    progname = argv[0];

    wait_for_socket = argc > 1 && strcmp(argv[1], "--wait") == 0;
    if (wait_for_socket) {
        argv++;
        argc--;
    }

    if (argc < 2) {
        usage(progname);
        exit(5);
    }
#ifdef MTK_FAT_ON_NAND
        if (!strcmp(argv[1], "fatcreation")) {
            char fatimgfilepath[200];
            struct statfs st;
            char first_boot[PROPERTY_VALUE_MAX];
            const char *fat_mnt;
            const char *fat_filename;
            pid_t pid;

            fat_mnt = FAT_PARTITION_MOUNT_POINT;
            fat_filename = FAT_IMG_NAME;

            snprintf(fatimgfilepath, sizeof(fatimgfilepath), "%s/%s", fat_mnt, fat_filename);
            memset(first_boot, 0x0, sizeof(first_boot));
            property_get("persist.vold.auto_format_intsd", first_boot, "1");
            printf("first_boot:%s\n", first_boot);
            if (!strcmp(first_boot, "1")) {
                if (access(fatimgfilepath, F_OK) != 0) {
    CREAT_FAT_IMG:
                    printf("%s does not exist, create FAT image\n", fatimgfilepath);
                    if (statfs(fat_mnt, &st) < 0) {
                        printf("get fs stat fail(%s) %s(%d)\n", fat_mnt, strerror(errno), errno);
                    }
                    else {
                        printf("%s free space:%llu f_bfree:%llu f_bsize:%u\n",
                            fat_mnt, (long long)((long long)st.f_bfree * (long long)st.f_bsize), st.f_bfree, st.f_bsize);
                        if ((long long)((long long)st.f_bfree * (long long)st.f_bsize) < (long long)FAT_IMG_MIN) {
                            printf("WARNING:%s free space:%llu, FAT_IMG_MIN:%d\n",
                                fat_mnt, (long long)((long long)st.f_bfree * (long long)st.f_bsize), FAT_IMG_MIN);
                        }
                        else {
                            int flag = O_RDWR|O_CREAT;
                            int fatimgfd;

                            fatimgfd = open(fatimgfilepath, flag, S_IRUSR|S_IWUSR);
                            if (fatimgfd < 0) {
                                printf("Fail to create %s %s(%d)", fatimgfilepath, strerror(errno), errno);
                            }

#ifdef FAT_TEST_IMG
                            if (ftruncate64(fatimgfd, FAT_TEST_IMG) < 0) {
#else
                            if (ftruncate64(fatimgfd, (st.f_bfree*st.f_bsize-RESERVE_SIZE)) < 0) {
#endif
                                printf("Fail to enlarge %s %s(%d)", fatimgfilepath, strerror(errno), errno);
                            }
                            fsync(fatimgfd);
                            close(fatimgfd);
                        }
                    }
                }
                else {
                    printf("First boot but %s has been created. Maybe power lost last time?\n", fatimgfilepath);
                }
            }
            else {
                if (access(fatimgfilepath, F_OK) != 0) {
                    printf("oops! FAT image has to be alreay created.Something wrong!Create again!\n");
                    property_set("persist.vold.auto_format_intsd", "1");
                    goto CREAT_FAT_IMG;
                }
                else
                    printf("%s has been created\n", fatimgfilepath);
            }
            return 0;
        }
#endif

    const char* sockname = "vold";
    if (!strcmp(argv[1], "cryptfs")) {
        sockname = "cryptd";
    }

    while ((sock = socket_local_client(sockname,
                                 ANDROID_SOCKET_NAMESPACE_RESERVED,
                                 SOCK_STREAM)) < 0) {
        if (!wait_for_socket) {
            fprintf(stdout, "Error connecting to %s: %s\n", sockname, strerror(errno));
            exit(4);
        } else {
            usleep(10000);
        }
    }

    if (!strcmp(argv[1], "monitor")) {
        exit(do_monitor(sock, 0));
    } else {
        exit(do_cmd(sock, argc, argv));
    }
}

static int do_cmd(int sock, int argc, char **argv) {
    int seq = getpid();

    std::string cmd(android::base::StringPrintf("%d ", seq));
    for (int i = 1; i < argc; i++) {
        if (!strchr(argv[i], ' ')) {
            cmd.append(argv[i]);
        } else {
            cmd.push_back('\"');
            cmd.append(argv[i]);
            cmd.push_back('\"');
        }

        if (i < argc - 1) {
            cmd.push_back(' ');
        }
    }

    if ((write(sock, cmd.c_str(), cmd.length() + 1)) < 0) {
        fprintf(stderr, "Failed to write command: %s\n", strerror(errno));
        return errno;
    }

    return do_monitor(sock, seq);
}

static int do_monitor(int sock, int stop_after_seq) {
    char buffer[4096];
    int timeout = kCommandTimeoutMs;

    if (stop_after_seq == 0) {
        fprintf(stderr, "Connected to vold\n");
        timeout = -1;
    }

    while (1) {
        struct pollfd poll_sock = { sock, POLLIN, 0 };
        int rc = TEMP_FAILURE_RETRY(poll(&poll_sock, 1, timeout));
        if (rc == 0) {
            fprintf(stderr, "Timeout waiting for %d\n", stop_after_seq);
            return ETIMEDOUT;
        } else if (rc < 0) {
            fprintf(stderr, "Failed during poll: %s\n", strerror(errno));
            return errno;
        }

        if (!(poll_sock.revents & POLLIN)) {
            fprintf(stderr, "No data; trying again\n");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        rc = TEMP_FAILURE_RETRY(read(sock, buffer, sizeof(buffer)));
        if (rc == 0) {
            fprintf(stderr, "Lost connection, did vold crash?\n");
            return ECONNRESET;
        } else if (rc < 0) {
            fprintf(stderr, "Error reading data: %s\n", strerror(errno));
            return errno;
        }

        int offset = 0;
        for (int i = 0; i < rc; i++) {
            if (buffer[i] == '\0') {
                char* res = buffer + offset;
                fprintf(stdout, "%s\n", res);

                int code = atoi(strtok(res, " "));
                if (code >= 200 && code < 600) {
                    int seq = atoi(strtok(nullptr, " "));
                    if (seq == stop_after_seq) {
                        if (code == 200) {
                            return 0;
                        } else {
                            return code;
                        }
                    }
                }

                offset = i + 1;
            }
        }
    }
    return EIO;
}

static void usage(char *progname) {
    fprintf(stderr,
            "Usage: %s [--wait] <monitor>|<cmd> [arg1] [arg2...]\n", progname);
}

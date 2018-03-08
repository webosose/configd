// Copyright (c) 2014-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "Environment.h"
#include "Manager.h"
#include "setting/Setting.h"
#include "service/ls2/LS2BusFactory.h"
#include "util/Logger.hpp"

void segfault_sig_action(int signal, siginfo_t *si, void *arg)
{
    printf("\n\n==== SEGMENTATION FAULT (%p) ====\n", si->si_addr);
    Manager::getInstance()->printDebug();
    exit(1);
}

void init_signal_handlers()
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = segfault_sig_action;
    sa.sa_flags = SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);
}

bool lock_process()
{
    pid_t pid;
    int fd;
    int result;
    char pidStr[16];
    int pidStrLen;
    bool retValue = true;

    pid = getpid();

    // open or create the lock file
    fd = ::open(CONFIGD_PID_FILE_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd < 0) {
        return false;
    }

    // use a POSIX advisory file lock as a mutex
    result = lockf(fd, F_TLOCK, 0);

    if (result < 0) {
        close(fd);
        return false;
    }

    // remove the old pid number data
    result = ftruncate(fd, 0);

    if (result < 0) {
        retValue = false;
    }

    // write the pid to the file
    snprintf(pidStr, sizeof(pidStr), "%d\n", pid);
    pidStrLen = (int) strlen(pidStr);
    result = (int) write(fd, pidStr, (size_t) pidStrLen);

    if (result < pidStrLen) {
        retValue = false;
    }

    close(fd);
    return retValue;
}

int main(int argc, char *argv[])
{
    // TODO both call should be moved into Logging Module
    PmLogGetContext("configd", &confdServiceLogContext);
    if (kPmLogErr_None == PmLogGetContext("configd-lib", &confdLibContext)) {
        PmLogSetLibContext(confdLibContext);
    }

    Setting::getInstance().printSetting();

    init_signal_handlers();

    Logger::info(MSGID_MAIN, LOG_PREPIX_FORMAT "Lock process", LOG_PREPIX_ARGS);
    if (!lock_process()) {
        Logger::error(MSGID_MAIN, LOG_PREPIX_FORMAT "Configd is already running", LOG_PREPIX_ARGS);
        return 0;
    }

    Logger::info(MSGID_MAIN, LOG_PREPIX_FORMAT "Set Bus Factory Instance", LOG_PREPIX_ARGS);
    AbstractBusFactory::setInstance(LS2BusFactory::getInstance());

    try {
        Logger::info(MSGID_MAIN, LOG_PREPIX_FORMAT "Manager Initialization", LOG_PREPIX_ARGS);
        Manager::getInstance()->initialize();
    } catch (std::exception& e) {
        Logger::error(MSGID_MAIN, LOG_PREPIX_FORMAT "Exception : %s", LOG_PREPIX_ARGS, e.what());
    }

    while (true) {
        try {
            Logger::info(MSGID_MAIN, LOG_PREPIX_FORMAT "Run Manager", LOG_PREPIX_ARGS);
            Manager::getInstance()->run();
        } catch (std::exception& e) {
            Logger::error(MSGID_MAIN, LOG_PREPIX_FORMAT "Exception : %s", LOG_PREPIX_ARGS, e.what());
            sleep(2);
        }
    }

    // TODO: shutting down properly
    return 0;
}

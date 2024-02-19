/*
 * Copyright (C) 2017 The Android Open Source Project
 * Copyright (C) 2017 The LineageOS Project
 * Copyright (C) 2018 TeamNexus
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

#define LOG_TAG "powerpulse-service"
// #define LOG_NDEBUG 0

#include <hidl/LegacySupport.h>
#include <hidl/HidlTransportSupport.h>
#include <log/log.h>

#include "Power.h"

#include <libpowerpulse.h>

using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;
using android::status_t;
using android::OK;

using android::hardware::power::V1_3::implementation::Power;

int main() {
	status_t status;
	android::sp<Power> service = nullptr;

    ALOGI("Initializing PowerPulse...");
    PowerPulse_Init();

	ALOGI("Initializing HAL...");
    service = new Power();
    if (service == nullptr) {
        ALOGE("Could not create an instance of the HAL!");
        goto shutdown;
    }

    ALOGI("Configuring the RPC thread pool...");
    configureRpcThreadpool(1, true /*callerWillJoin*/);

    ALOGI("Registering as a system service...");
    status = service->registerAsSystemService();
    if (status != OK) {
        ALOGE("Registering as a system service failed! (%d)", status);
        goto shutdown;
    }

    ALOGI("Joining the RPC thread pool...");
    joinRpcThreadpool();

shutdown:
    ALOGE("Good-bye! :D");
    return 1;
}

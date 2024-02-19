/*
 * Copyright (C) 2016 The Android Open Source Project
 * Copyright (C) 2018 The LineageOS Project
 * Copyright (C) 2018 TeamNexus
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "powerpulse"
// #define LOG_NDEBUG 0

#include <unistd.h>
#include <sys/wait.h>
#include <cstdio>

#include <log/log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#include "Power.h"

#include <libpowerpulse.h>

namespace android {
namespace hardware {
namespace power {
namespace V1_3 {
namespace implementation {

Power::Power()
{
	ALOGV("%s: enter;", __func__);

	//
	// defaults
	//
	mIsInteractive = false;
	mRequestedProfile = SecPowerProfiles::INVALID;
	mCurrentProfile = SecPowerProfiles::INVALID;
	mVariant = SecDeviceVariant::UNKNOWN;
	mTouchControlPath = "";
	mTouchkeysEnabled = true;
	mIsDT2WEnabled = false;

	ALOGV("%s: exit;", __func__);
}

status_t Power::registerAsSystemService() {
	status_t ret = 0;

	ret = IPower::registerAsService();
	LOG_ALWAYS_FATAL_IF(ret != 0, "%s: failed to register IPower HAL Interface as service (%d), aborting...", __func__, ret);
	ALOGI("%s: registered IPower HAL Interface!", __func__);

#ifdef POWER_HAS_LINEAGE_HINTS
	ret = ILineagePower::registerAsService();
	LOG_ALWAYS_FATAL_IF(ret != 0, "%s: failed to register ILineagePower HAL Interface as service (%d), aborting...", __func__, ret);
	ALOGI("%s: registered ILineagePower HAL Interface!", __func__);
#else
	ALOGI("%s: current ROM does not support ILineagePower HAL Interface!", __func__);
#endif

	return 0;
}

Power::~Power() { }

// Methods from ::android::hardware::power::V1_3::IPower follow.
Return<void> Power::setInteractive(bool interactive) {
	PowerPulse_SetInteractive(interactive);
	return Void();
}

Return<void> Power::powerHint(PowerHint hint, int32_t data)  {
	GoInt32 powerHint = (GoInt32) hint;
	GoInt32 powerData = (GoInt32) data;
	PowerPulse_SetPowerHint(powerHint, powerData);
	return Void();
}

Return<void> Power::setFeature(Feature feature, bool activate)  {
	int32_t feat = (int32_t) feature;
	PowerPulse_SetFeature(feat, activate);
	return Void();
}

Return<void> Power::getPlatformLowPowerStats(getPlatformLowPowerStats_cb _hidl_cb)  {
	ALOGV("%s: enter;", __func__);

	hidl_vec<PowerStatePlatformSleepState> states;
	_hidl_cb(states, Status::SUCCESS);

	ALOGV("%s: exit;", __func__);
	return Void();
}

// Methods from ::vendor::lineage::power::V1_0::ILineagePower follow.
#ifdef POWER_HAS_LINEAGE_HINTS
Return<int32_t> Power::getFeature(LineageFeature feature)  {
	uint32_t feat = (uint32_t) feature;
	int32_t resp = PowerPulse_GetFeature(feat);
	return resp;
}
#endif

bool Power::isModuleEnabled(string module) {
	return (GetProperty("sys.power." + module, "true") == "true");
}

} // namespace implementation
}  // namespace V1_3
}  // namespace power
}  // namespace hardware
}  // namespace android

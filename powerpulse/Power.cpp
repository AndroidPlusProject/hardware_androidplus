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
namespace V1_0 {
namespace implementation {

Power::Power()
{
	ALOGV("%s: enter;", __func__);
	power_lock();

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

// Methods from ::android::hardware::power::V1_0::IPower follow.
Return<void> Power::setInteractive(bool interactive) {
	ALOGV("%s: enter; interactive=%d", __func__, interactive ? 1 : 0);
	power_lock();

	this->mIsInteractive = interactive;

	if (!interactive) {
		setProfile(SecPowerProfiles::SCREEN_OFF);
	} else {
		// reset to requested- or fallback-profile
		resetProfile();
	}
	return Void();
}

Return<void> Power::powerHint(PowerHint hint, int32_t data)  {
	ALOGV("%s: enter; hint=%d, data=%d", __func__, hint, data);
	power_lock();

	switch_uint32_t (hint)
	{
		/*
		 * Profiles
		 */
#ifdef POWER_HAS_LINEAGE_HINTS
		case_uint32_t (LineagePowerHint::SET_PROFILE):
		{
			ALOGV("%s: LineagePowerHint::SET_PROFILE(%d)", __func__, data);
			mRequestedProfile = static_cast<SecPowerProfiles>(data);
			setProfile(mRequestedProfile);
			break;
		}
#endif
		case_uint32_t (PowerHint::LOW_POWER):
		{
			ALOGV("%s: PowerHint::LOW_POWER(%d)", __func__, data);
			if (data) {
				setProfile(SecPowerProfiles::POWER_SAVE);
			} else {
				resetProfile();
			}
			break;
		}
		case_uint32_t (PowerHint::SUSTAINED_PERFORMANCE):
		{
			ALOGV("%s: PowerHint::SUSTAINED_PERFORMANCE(%d)", __func__, data);
			if (data) {
				setProfile(SecPowerProfiles::HIGH_PERFORMANCE);
			} else {
				resetProfile();
			}
			break;
		}
		case_uint32_t (PowerHint::VR_MODE):
		{
			ALOGV("%s: PowerHint::VR_MODE(%d)", __func__, data);
			if (data) {
				setProfile(SecPowerProfiles::HIGH_PERFORMANCE);
			} else {
				resetProfile();
			}
			break;
		}

		/*
		 * Interaction/Boosting
		 */
		case_uint32_t (PowerHint::INTERACTION):
		{
			ALOGV("%s: PowerHint::INTERACTION(%d)", __func__, data);
			boostpulse(data);
			break;
		}
		case_uint32_t (PowerHint::LAUNCH):
		{
			ALOGV("%s: PowerHint::LAUNCH(%d)", __func__, data);
			break;
		}
		case_uint32_t (PowerHint::VSYNC):
		{
			ALOGV("%s: PowerHint::VSYNC(%d)", __func__, data);
			break;
		}
	}

	// ALOGV("%s: exit;", __func__);
	return Void();
}

Return<void> Power::setFeature(Feature feature, bool activate)  {
	ALOGV("%s: enter; feature=%d, activate=%d", __func__, feature, activate ? 1 : 0);
	power_lock();

	switch_uint32_t (feature)
	{
		case_uint32_t (Feature::POWER_FEATURE_DOUBLE_TAP_TO_WAKE):
		{
			mIsDT2WEnabled = activate;
			setDT2WState();
			break;
		}
	}

	ALOGV("%s: exit;", __func__);
	return Void();
}

Return<void> Power::getPlatformLowPowerStats(getPlatformLowPowerStats_cb _hidl_cb)  {
	ALOGV("%s: enter;", __func__);
	power_lock();

	hidl_vec<PowerStatePlatformSleepState> states;
	_hidl_cb(states, Status::SUCCESS);

	ALOGV("%s: exit;", __func__);
	return Void();
}

// Methods from ::vendor::lineage::power::V1_0::ILineagePower follow.
#ifdef POWER_HAS_LINEAGE_HINTS
Return<int32_t> Power::getFeature(LineageFeature feature)  {
	ALOGV("%s: enter; feature=%d", __func__, feature);
	power_lock();

	switch_uint32_t (feature)
	{
		case_uint32_t (LineageFeature::SUPPORTED_PROFILES):
		{
			return static_cast<int>(SecPowerProfiles::MAX_PROFILES);
		}

		case_uint32_t (Feature::POWER_FEATURE_DOUBLE_TAP_TO_WAKE):
		{
			return 0;
		}
	}

	ALOGV("%s: exit;", __func__);
	return -1;
}
#endif

// Private Methods
void Power::boostpulse(int duration) {
	std::lock_guard<std::mutex> autolock(mBoostpulseLock);

	if (duration <= 0) {
		duration = (1000 / 60) * 10;
	}
	duration *= 1000;
}

void Power::setProfile(SecPowerProfiles profile) {
	// check if user disabled power-profiles
	if (!isModuleEnabled("profiles")) {
		ALOGI("power profiles disabled by user!");
		return;
	}

	char* profileName = (char*) SecPowerProfilesToString(profile);
	if (SecPowerProfilesToString(mCurrentProfile) == profileName) {
		ALOGI("%s: already applied profile %s (%d)", __func__, profileName, profile);
		return;
	}
	mCurrentProfile = profile;

 	ALOGI("%s: applying profile %s (%d)", __func__, profileName, profile);
	PowerPulse_SetProfile(profileName);
}

void Power::resetProfile() {
	if (mRequestedProfile == SecPowerProfiles::INVALID) {
		setProfile(SecPowerProfiles::BALANCED);
	} else {
		setProfile(mRequestedProfile);
	}
}

void Power::setDT2WState() {
	if (mIsDT2WEnabled) {

	} else {

	}
}

bool Power::isModuleEnabled(string module) {
	return (GetProperty("sys.power." + module, "true") == "true");
}

} // namespace implementation
}  // namespace V1_0
}  // namespace power
}  // namespace hardware
}  // namespace android

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

#ifndef ZERO_HARDWARE_POWER_V1_3_POWER_H
#define ZERO_HARDWARE_POWER_V1_3_POWER_H

#include <android/hardware/power/1.3/IPower.h>

#ifdef POWER_HAS_LINEAGE_HINTS
#include <vendor/lineage/power/1.0/ILineagePower.h>
#endif

#include <android-base/properties.h>

#include <hidl/Status.h>
#include <hidl/MQDescriptor.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#include <chrono>
#include <thread>

using std::ostringstream;

#define POWER_DT2W_ENABLED                "/sys/class/dt2w/enabled"
#define POWER_TOUCHKEYS_ENABLED           "/sys/class/sec/sec_touchkey/input/enabled"
#define POWER_TOUCHKEYS_BRIGHTNESS        "/sys/class/sec/sec_touchkey/brightness"
#define POWER_FINGERPRINT_PM		  "/sys/class/fingerprint/fingerprint/pm"
#define POWER_FINGERPRINT_WAKELOCKS       "/sys/class/fingerprint/fingerprint/wakelocks"

#define POWER_TOUCHSCREEN_NAME            "/sys/class/input/input1/name"
#define POWER_TOUCHSCREEN_NAME_EXPECT     "sec_touchscreen"
#define POWER_TOUCHSCREEN_ENABLED_FLAT    "/sys/class/input/input1/enabled"
#define POWER_TOUCHSCREEN_ENABLED_EDGE    "/sys/class/input/input0/enabled"

#define POWER_PROFILE_POLLING_FILE        "/data/.power-profile"

/*
 * Quick-Casts
 */
#define cint(data)  static_cast<int>(data)

namespace android {
namespace hardware {
namespace power {
namespace V1_3 {
namespace implementation {

using ::android::hardware::power::V1_0::Feature;
using ::android::hardware::power::V1_0::IPower;
using ::android::hardware::power::V1_0::PowerHint;
using ::android::hardware::power::V1_0::PowerStatePlatformSleepState;
using ::android::hardware::power::V1_0::Status;
#ifdef POWER_HAS_LINEAGE_HINTS
using ::vendor::lineage::power::V1_0::ILineagePower;
using ::vendor::lineage::power::V1_0::LineageFeature;
using ::vendor::lineage::power::V1_0::LineagePowerHint;
#endif

using ::android::base::GetProperty;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::sp;
using ::android::status_t;

using ::std::string;
using ::std::thread;

#ifdef LOCK_PROTECTION
  #define power_lock() \
      std::lock_guard<std::mutex> autolock(mLock)
#else
	#define power_lock()
#endif

#define switch_uint32_t(_data)    switch (static_cast<uint32_t>(_data))
#define case_uint32_t(_feature)    case static_cast<uint32_t>(_feature)

enum class SecPowerProfiles : int32_t {
	INVALID          = (-0x7FFFFFFF - 1),
	SCREEN_OFF       = -1,
	POWER_SAVE       = 0,
	BALANCED         = 1,
	HIGH_PERFORMANCE = 2,
	BIAS_POWER_SAVE  = 3,
	BIAS_PERFORMANCE = 4,
	MAX_PROFILES     = 5,
	MAX              = 6
};

inline const char* SecPowerProfilesToString(SecPowerProfiles v)
{
	switch (v)
	{
		case SecPowerProfiles::INVALID:          return "invalid";
		case SecPowerProfiles::SCREEN_OFF:       return "screen_off";
		case SecPowerProfiles::POWER_SAVE:       return "battery_saver";
		case SecPowerProfiles::BALANCED:         return "balanced";
		case SecPowerProfiles::HIGH_PERFORMANCE: return "performance";
		case SecPowerProfiles::BIAS_POWER_SAVE:  return "efficiency";
		case SecPowerProfiles::BIAS_PERFORMANCE: return "quick";
		case SecPowerProfiles::MAX_PROFILES:     return "max_profiles";
		case SecPowerProfiles::MAX:              return "max";
		default:                                 return "unknown";
	}
}

enum class SecDeviceVariant : int32_t {
	UNKNOWN = 0,
	FLAT = 1,
	EDGE = 2,
};

struct Power :
      public IPower
#ifdef POWER_HAS_LINEAGE_HINTS
    , public ILineagePower
#endif
{

	Power();
	~Power();

	status_t registerAsSystemService();

	// Methods from ::android::hardware::power::V1_3::IPower follow.
	Return<void> setInteractive(bool interactive)  override;
	Return<void> powerHint(PowerHint hint, int32_t data)  override;
	Return<void> setFeature(Feature feature, bool activate)  override;
	Return<void> getPlatformLowPowerStats(getPlatformLowPowerStats_cb _hidl_cb)  override;

	// Methods from ::vendor::lineage::power::V1_3::ILineagePower follow.
#ifdef POWER_HAS_LINEAGE_HINTS
	Return<int32_t> getFeature(LineageFeature feature)  override;
#endif

private:
#ifdef LOCK_PROTECTION
	mutable std::mutex mLock;
#endif
	mutable std::mutex mBoostpulseLock;

	// Stores the current interactive-state of the device
	// Default to <false>.
	bool mIsInteractive;

	// profiles requested by SET_PROFILE-hint.
	// Default to <INVALID>.
	SecPowerProfiles mRequestedProfile;

	// profile currently in use. may be set by every power-method.
	// Default to <INVALID>.
	SecPowerProfiles mCurrentProfile;

	// The device-variant this power-service is currently running on
	// Default to <UNKNOWN>.
	SecDeviceVariant mVariant;

	// The path to control the state of the touchscreen. May vary between
	// different variants.
	// Default to <"">.
	string mTouchControlPath;

	// Stores the current state of the touchkeys to prevent accidental
	// enabling if user decidec to use on-screen-navbar and disabled them
	// Default to <true>.
	bool mTouchkeysEnabled;

	// Stores the user-set doubletap2wake-state
	// Default to <false>.
	bool mIsDT2WEnabled;

	//
	// Private methods
	//

	// Sends a boostpulse request to the cpugov if supported.
	void boostpulse(int duration);

	// Set the current profile to [profile]. Also updates mCurrentProfile.
	// Default to UNKNOWN
	void setProfile(SecPowerProfiles profile);

	// Either resets the current profile to mRequestedProfile or
	// falls back to BALANCED if mRequestedProfile is set to INVALID.
	void resetProfile();

	// updates the current state the doubletap2wake-capability. uses the
	// global member [mIsDT2WEnabled] to determine the new state
	void setDT2WState();

	// fetches the correct property and checks if the user disable
	// a specific power-module.
	bool isModuleEnabled(string module);
};

}  // namespace implementation
}  // namespace V1_3
}  // namespace power
}  // namespace hardware
}  // namespace android

#endif  // ZERO_HARDWARE_POWER_V1_3_POWER_H

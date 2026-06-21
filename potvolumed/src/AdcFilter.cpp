#define LOG_TAG "potvolumed"


#include "AdcFilter.h"

#include <cmath>
#include <log/log.h>

int AdcFilter::update(int rawValue, bool* changed) {
    *changed = false;

    // Seed on first call to avoid a spurious ramp-up from -1 at boot
    if (mEmaValue < 0.0f) {
        mEmaValue   = static_cast<float>(rawValue);
        mLastStable = rawValue;
        *changed    = true;
        ALOGD("AdcFilter: seeded at raw=%d", rawValue);
        return mLastStable;
    }

    // Stage 1: Exponential Moving Average — EMA = α×raw + (1−α)×EMA
    mEmaValue = kAlpha * static_cast<float>(rawValue)
              + (1.0f - kAlpha) * mEmaValue;

    int filtered = static_cast<int>(std::round(mEmaValue));

    // Stage 2: Dead zone — only notify when movement exceeds threshold
    if (std::abs(filtered - mLastStable) >= kDeadZone) {
        ALOGD("AdcFilter: raw=%d ema=%.1f stable=%d→%d (Δ=%d)",
              rawValue, mEmaValue, mLastStable, filtered,
              filtered - mLastStable);
        mLastStable = filtered;
        *changed    = true;
    }

    return mLastStable;
}

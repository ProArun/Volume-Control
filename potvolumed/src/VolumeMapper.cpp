#define LOG_TAG "potvolumed"

// Copyright (C) 2024 MyOEM
// SPDX-License-Identifier: Apache-2.0

#include "VolumeMapper.h"

#include <algorithm>
#include <cmath>
#include <log/log.h>

int VolumeMapper::toVolumeIndex(int adcValue) const {
    adcValue = std::max(0, std::min(adcValue, kAdcMax));

    float ratio = static_cast<float>(adcValue) / static_cast<float>(kAdcMax);
    int index   = static_cast<int>(std::round(ratio * static_cast<float>(kVolumeMax)));

    index = std::max(0, std::min(index, kVolumeMax));

    ALOGD("VolumeMapper: adc=%d → ratio=%.3f → index=%d/%d",
          adcValue, ratio, index, kVolumeMax);

    return index;
}

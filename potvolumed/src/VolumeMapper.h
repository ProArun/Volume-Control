// Copyright (C) 2024 MyOEM
// SPDX-License-Identifier: Apache-2.0
#pragma once

class VolumeMapper {
public:
    // Maps a 10-bit ADC value (0–1023) to an AUDIO_STREAM_MUSIC index (0–15)
    int toVolumeIndex(int adcValue) const;

private:
    static constexpr int kAdcMax    = 1023;
    static constexpr int kVolumeMax = 15;
};

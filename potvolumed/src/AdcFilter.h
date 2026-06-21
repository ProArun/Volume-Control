// Copyright (C) 2024 MyOEM
// SPDX-License-Identifier: Apache-2.0
#pragma once

class AdcFilter {
public:
    int update(int rawValue, bool* changed);

private:
    // EMA smoothing factor — 0.2 balances responsiveness with noise rejection
    static constexpr float kAlpha    = 0.2f;
    // Dead zone in ADC counts — suppresses sub-step jitter without deadening the knob
    static constexpr int   kDeadZone = 8;

    float mEmaValue{-1.0f};   // -1 = not yet seeded
    int   mLastStable{-1};
};

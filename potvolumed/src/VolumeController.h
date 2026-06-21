// Copyright (C) 2024 MyOEM
// SPDX-License-Identifier: Apache-2.0
#pragma once

class VolumeController {
public:
    VolumeController();
    ~VolumeController();

    bool open();
    void close();
    bool setVolume(int volumeIndex);

private:
    bool sendKeyEvent(int keyCode, int value);
    bool sendSyncEvent();

    int mFd{-1};
    int mLastIndex{-1};   // -1 = boot state: record reference without sending events
};

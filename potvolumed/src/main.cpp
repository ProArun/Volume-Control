#define LOG_TAG "potvolumed"

// Copyright (C) 2024 MyOEM
// SPDX-License-Identifier: Apache-2.0

#include <log/log.h>

#include <csignal>
#include <atomic>
#include <chrono>
#include <thread>

#include "SpiReader.h"
#include "AdcFilter.h"
#include "VolumeMapper.h"
#include "VolumeController.h"

static constexpr char kSpiDevice[]    = "/dev/spidev0.0";
static constexpr int  kAdcChannel     = 0;
static constexpr int  kPollIntervalMs = 50;   // 20 Hz

static std::atomic<bool> gRunning{true};

static void onSignal(int /*signum*/) {
    gRunning = false;
}

int main() {
    ALOGI("potvolumed starting (spi=%s ch=%d poll=%dms)",
          kSpiDevice, kAdcChannel, kPollIntervalMs);

    signal(SIGTERM, onSignal);
    signal(SIGINT,  onSignal);

    SpiReader        spi(kSpiDevice, kAdcChannel);
    AdcFilter        filter;
    VolumeMapper     mapper;
    VolumeController controller;

    if (!spi.open()) {
        ALOGE("Cannot open SPI device %s — daemon cannot start", kSpiDevice);
        return 1;
    }

    if (!controller.open()) {
        ALOGE("Cannot open uinput device — daemon cannot start");
        return 1;
    }

    ALOGI("SPI and uinput ready. Entering poll loop.");

    while (gRunning) {
        int raw = spi.read();
        if (raw < 0) {
            ALOGE("SPI read error — skipping cycle");
            std::this_thread::sleep_for(std::chrono::milliseconds(kPollIntervalMs));
            continue;
        }

        bool changed = false;
        int  stable  = filter.update(raw, &changed);

        if (changed) {
            int index = mapper.toVolumeIndex(stable);
            controller.setVolume(index);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(kPollIntervalMs));
    }

    ALOGI("potvolumed shutting down cleanly");
    return 0;
}

#define LOG_TAG "potvolumed"

// Copyright (C) 2024 MyOEM
// SPDX-License-Identifier: Apache-2.0

#include "SpiReader.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <cerrno>
#include <cstring>
#include <log/log.h>

static constexpr uint32_t kSpiSpeedHz     = 1'000'000;
static constexpr uint8_t  kSpiMode        = SPI_MODE_0;
static constexpr uint8_t  kSpiBitsPerWord = 8;

SpiReader::SpiReader(const std::string& device, int channel)
    : mDevice(device), mChannel(channel) {}

SpiReader::~SpiReader() {
    close();
}

bool SpiReader::open() {
    mFd = ::open(mDevice.c_str(), O_RDWR);
    if (mFd < 0) {
        ALOGE("open(%s) failed: %s", mDevice.c_str(), strerror(errno));
        return false;
    }

    if (ioctl(mFd, SPI_IOC_WR_MODE, &kSpiMode) < 0) {
        ALOGE("SPI_IOC_WR_MODE failed: %s", strerror(errno));
        ::close(mFd); mFd = -1;
        return false;
    }

    if (ioctl(mFd, SPI_IOC_WR_BITS_PER_WORD, &kSpiBitsPerWord) < 0) {
        ALOGE("SPI_IOC_WR_BITS_PER_WORD failed: %s", strerror(errno));
        ::close(mFd); mFd = -1;
        return false;
    }

    if (ioctl(mFd, SPI_IOC_WR_MAX_SPEED_HZ, &kSpiSpeedHz) < 0) {
        ALOGE("SPI_IOC_WR_MAX_SPEED_HZ failed: %s", strerror(errno));
        ::close(mFd); mFd = -1;
        return false;
    }

    ALOGI("SPI device %s opened: mode=%d bits=%d speed=%uHz",
          mDevice.c_str(), kSpiMode, kSpiBitsPerWord, kSpiSpeedHz);
    return true;
}

void SpiReader::close() {
    if (mFd >= 0) {
        ::close(mFd);
        mFd = -1;
    }
}

int SpiReader::read() {
    if (mFd < 0) {
        ALOGE("read() called but SPI device is not open");
        return -1;
    }

    // 3-byte MCP3008 transaction: start bit, channel select, result clock-out
    uint8_t tx[3] = {
        0x01,
        static_cast<uint8_t>(0x80 | ((mChannel & 0x07) << 4)),
        0x00
    };
    uint8_t rx[3] = {0x00, 0x00, 0x00};

    struct spi_ioc_transfer tr{};
    tr.tx_buf        = reinterpret_cast<unsigned long>(tx);
    tr.rx_buf        = reinterpret_cast<unsigned long>(rx);
    tr.len           = sizeof(tx);
    tr.speed_hz      = kSpiSpeedHz;
    tr.bits_per_word = kSpiBitsPerWord;
    tr.delay_usecs   = 0;

    if (ioctl(mFd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        ALOGE("SPI_IOC_MESSAGE ioctl failed: %s", strerror(errno));
        return -1;
    }

    // rx[1] bits[1:0] = ADC bits[9:8], rx[2] = ADC bits[7:0]
    int value = ((rx[1] & 0x03) << 8) | rx[2];

    ALOGV("SPI raw: tx=[%02X %02X %02X] rx=[%02X %02X %02X] value=%d",
          tx[0], tx[1], tx[2], rx[0], rx[1], rx[2], value);

    return value;
}

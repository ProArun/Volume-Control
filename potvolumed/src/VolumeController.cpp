#define LOG_TAG "potvolumed"


#include "VolumeController.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <log/log.h>

VolumeController::VolumeController()  = default;
VolumeController::~VolumeController() { close(); }

bool VolumeController::open() {
    mFd = ::open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (mFd < 0) {
        ALOGE("open(/dev/uinput) failed: %s", strerror(errno));
        return false;
    }

    if (ioctl(mFd, UI_SET_EVBIT, EV_KEY) < 0 ||
        ioctl(mFd, UI_SET_EVBIT, EV_SYN) < 0) {
        ALOGE("UI_SET_EVBIT failed: %s", strerror(errno));
        ::close(mFd); mFd = -1;
        return false;
    }

    if (ioctl(mFd, UI_SET_KEYBIT, KEY_VOLUMEUP)  < 0 ||
        ioctl(mFd, UI_SET_KEYBIT, KEY_VOLUMEDOWN) < 0) {
        ALOGE("UI_SET_KEYBIT failed: %s", strerror(errno));
        ::close(mFd); mFd = -1;
        return false;
    }

    struct uinput_setup usetup{};
    strncpy(usetup.name, "PotVolume Knob", UINPUT_MAX_NAME_SIZE - 1);
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor  = 0x1209;
    usetup.id.product = 0x0001;
    usetup.id.version = 1;

    if (ioctl(mFd, UI_DEV_SETUP, &usetup) < 0) {
        ALOGE("UI_DEV_SETUP failed: %s", strerror(errno));
        ::close(mFd); mFd = -1;
        return false;
    }

    if (ioctl(mFd, UI_DEV_CREATE) < 0) {
        ALOGE("UI_DEV_CREATE failed: %s", strerror(errno));
        ::close(mFd); mFd = -1;
        return false;
    }

    ALOGI("Virtual input device 'PotVolume Knob' created on /dev/uinput");
    return true;
}

void VolumeController::close() {
    if (mFd >= 0) {
        ioctl(mFd, UI_DEV_DESTROY);
        ::close(mFd);
        mFd = -1;
        ALOGI("Virtual input device destroyed");
    }
}

bool VolumeController::sendKeyEvent(int keyCode, int value) {
    struct input_event ev{};
    ev.type  = EV_KEY;
    ev.code  = static_cast<__u16>(keyCode);
    ev.value = value;
    if (write(mFd, &ev, sizeof(ev)) != sizeof(ev)) {
        ALOGE("write(EV_KEY code=%d val=%d) failed: %s", keyCode, value, strerror(errno));
        return false;
    }
    return true;
}

bool VolumeController::sendSyncEvent() {
    struct input_event ev{};
    ev.type  = EV_SYN;
    ev.code  = SYN_REPORT;
    ev.value = 0;
    if (write(mFd, &ev, sizeof(ev)) != sizeof(ev)) {
        ALOGE("write(EV_SYN) failed: %s", strerror(errno));
        return false;
    }
    return true;
}

bool VolumeController::setVolume(int volumeIndex) {
    if (mFd < 0) {
        ALOGE("setVolume called but uinput device is not open");
        return false;
    }

    // On first call, record boot position without sending any events
    if (mLastIndex < 0) {
        mLastIndex = volumeIndex;
        ALOGI("VolumeController: boot reference set to index=%d", volumeIndex);
        return true;
    }

    if (volumeIndex == mLastIndex) return true;

    int delta   = volumeIndex - mLastIndex;
    int keyCode = (delta > 0) ? KEY_VOLUMEUP : KEY_VOLUMEDOWN;
    int steps   = std::abs(delta);

    ALOGI("Volume: index %d → %d (Δ=%+d, %d × %s)",
          mLastIndex, volumeIndex, delta, steps,
          (delta > 0) ? "KEY_VOLUMEUP" : "KEY_VOLUMEDOWN");

    for (int i = 0; i < steps; i++) {
        if (!sendKeyEvent(keyCode, 1)) return false;
        if (!sendSyncEvent())          return false;
        if (!sendKeyEvent(keyCode, 0)) return false;
        if (!sendSyncEvent())          return false;
    }

    mLastIndex = volumeIndex;
    return true;
}

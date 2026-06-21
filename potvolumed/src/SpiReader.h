#pragma once

#include <string>

class SpiReader {
public:
    SpiReader(const std::string& device, int channel);
    ~SpiReader();

    bool open();
    void close();
    int  read();

private:
    std::string mDevice;
    int         mChannel;
    int         mFd{-1};
};

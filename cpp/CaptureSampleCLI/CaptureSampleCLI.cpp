#include <atomic>
#include <id3Devices/id3HighResTimer.h>
#include <id3DevicesCppWrapper/id3DevicesDeviceManager.hpp>
#include <id3DevicesCppWrapper/id3DevicesLicense.hpp>
#include <id3DevicesCppWrapper/id3DevicesCamera.hpp>
#include <format>
#include <iostream>

using namespace id3DevicesCppWrapper;

//#define LOOP_MODE_THREAD

std::atomic<bool> deviceAddedOk;
void deviceAddedCallback(void *context, int deviceId) {
    try {
        auto devInfo = DeviceManager::getDeviceInfo(deviceId);
        deviceAddedOk = true;
    }
    catch (DevicesException &e) {
        std::cout << e.what() << std::endl;
    }
}

bool waitForDevice(uint32_t delayMs) {
    auto timeout = id3HighResTimer_GetTickCountMS() + delayMs;
    for(;;) {
        id3HighResTimer_WaitMS(20);
#ifndef LOOP_MODE_THREAD
        DeviceManager::doEvent(); // only if message loop is set to none
#endif
        if (deviceAddedOk) {
            return true;
        }
        if (id3HighResTimer_GetTickCountMS() > timeout) {
            break;
        }
    }
    return false;
}

std::atomic<bool> stop_capture;
void deviceRemovedCallback(void *context, int deviceId) {
    deviceAddedOk = false;
    stop_capture = true;
}

void captureCallback(void *context) {
    if (context == nullptr) {
        return;
    }
    try {
        auto cameraChannel = (Camera *)context;
        CaptureImage image;
        bool available = cameraChannel->getCurrentFrame(image);
        if (available) {
            int frameNumber = image.getFrameCount();
            int64_t timestamp = image.getTimestamp();
            //last_frame_number = frameNumber;
            printf("frameNumber %6d: timestamp %lld\n", frameNumber, timestamp);

            int width  = image.getWidth();
            int height = image.getHeight();
            int stride = image.getStride();
            void *pixels = image.getPixels();
            printf("W %d H %d\n", width, height);
            std::cout << frameNumber << std::endl;

            //std::string filename = std::format("C:\\temp\\capture\\test\\capture_{:06}.jpg", frameNumber);
            //image.save(filename, 100);
            if (frameNumber > 10) {
                stop_capture = true;
            }
        }
    }
    catch (DevicesException &e) {
        std::cout << e.what() << std::endl;
    }
}

int main() {
    try {
        DeviceManager::initialize();
#ifdef LOOP_MODE_THREAD
        std::cout << "CaptureSampleCLI LOOP_MODE_THREAD ON" << std::endl;
        DeviceManager::configure(id3DevicesMessageLoopMode_Thread);
#else
        std::cout << "CaptureSampleCLI LOOP_MODE_THREAD OFF" << std::endl;
        DeviceManager::configure(id3DevicesMessageLoopMode_None);
#endif
    }
    catch (DevicesException &e) {
        std::cout << e.what() << std::endl;
    }

    try {
        DevicesLicense::checkLicense("c:/ProgramData/id3/id3FaceToolkit_v9.lic");
        DeviceManager::setDeviceAddedCallback(deviceAddedCallback, nullptr);
        DeviceManager::start();
        DeviceManager::loadPlugin("id3DevicesWebcam");
    }
    catch (DevicesException &e) {
        std::cout << e.what() << std::endl;
    }

    std::cout << "Wait for device" << std::endl;
    auto result = waitForDevice(10000);
    if (result) {
        try {
            auto devInfoList = DeviceManager::getDeviceInfoList();
            printf("found %d FingerPrint device(s)\n",devInfoList.getCount());
            Camera cameraChannel;
            auto device = devInfoList.get(0);
            cameraChannel.setCaptureCallback(captureCallback, &cameraChannel);
            cameraChannel.setDeviceRemovedCallback(deviceRemovedCallback, &cameraChannel);
            cameraChannel.openDevice(device.getDeviceId());
            bool loop = true;
            while (loop) {
#ifndef LOOP_MODE_THREAD
                DeviceManager::doEvent();
#endif
                auto state = cameraChannel.getDeviceState();
                switch (state) {
                    case id3DevicesDeviceState_NoDevice:
                    case id3DevicesDeviceState_DeviceError:
                        std::cout << "lost device" << std::endl;
                        loop = false;
                        break;
                    case id3DevicesDeviceState_DeviceReady: {
                        cameraChannel.startCapture();
                        break;
                    }
                    case id3DevicesDeviceState_CaptureInProgress: {
                        if (stop_capture) {
                            cameraChannel.stopCapture();
                            loop = false;
                        }
                        id3HighResTimer_WaitMS(20);
                        break;
                    }
                    default:
                        break;
                }
            }
        }
        catch (DevicesException &e) {
            std::cout << e.what() << std::endl;
        }
    }

    DeviceManager::stop();
    DeviceManager::dispose();
    return 0;
}

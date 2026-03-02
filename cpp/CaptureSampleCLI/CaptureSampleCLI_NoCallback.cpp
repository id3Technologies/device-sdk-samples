#include <id3Devices/id3HighResTimer.h>
#include <id3DevicesCppWrapper/id3DevicesDeviceManager.hpp>
#include <id3DevicesCppWrapper/id3DevicesLicense.hpp>
#include <id3DevicesCppWrapper/id3DevicesCamera.hpp>
#include <format>
#include <iostream>

using namespace id3DevicesCppWrapper;

//#define LOOP_MODE_THREAD

bool waitForDevice(uint32_t delayMs) {
    auto timeout = id3HighResTimer_GetTickCountMS() + delayMs;
    for(;;) {
        id3HighResTimer_WaitMS(20);
#ifndef LOOP_MODE_THREAD
        DeviceManager::doEvent(); // only if message loop is set to none
#endif
        try {
            auto deviceCount = DeviceManager::getDeviceCount();
            if (deviceCount >= 1) {
                return true;
            }
            if (id3HighResTimer_GetTickCountMS() > timeout) {
                break;
            }
        }
        catch (DevicesException &e) {
            std::cout << e.what() << std::endl;
        }
    }
    return false;
}

int main() {
    DeviceManager::initialize();
#ifdef LOOP_MODE_THREAD
    std::cout << "CaptureSampleCLI NoCallback LOOP_MODE_THREAD ON" << std::endl;
    DeviceManager::configure(id3DevicesMessageLoopMode_Thread);
#else
    std::cout << "CaptureSampleCLI NoCallback LOOP_MODE_THREAD OFF" << std::endl;
    DeviceManager::configure(id3DevicesMessageLoopMode_None);
#endif

    try {
        DevicesLicense::checkLicense("c:/ProgramData/id3/id3FaceToolkit_v9.lic");
        DeviceManager::start();
        DeviceManager::loadPlugin("id3DevicesWebcam");
    }
    catch (DevicesException &e) {
        std::cout << e.what() << std::endl;
    }

    std::cout << "Wait for device" << std::endl;
    auto result = waitForDevice(2000);
    if (result) {
        try {
            auto devInfoList = DeviceManager::getDeviceInfoList();
            printf("found %d FingerPrint device(s)\n",devInfoList.getCount());

            Camera cameraChannel;
            auto device = devInfoList.get(0);
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
#ifdef LOOP_MODE_THREAD
                        auto available = cameraChannel.waitForCapture(100, false);
#else
                        auto available = cameraChannel.waitForCapture(100, true);
#endif
                        if (available) {
                            CaptureImage image;
                            available = cameraChannel.getCurrentFrame(image);
                            if (available) {
                                int frameNumber = image.getFrameCount();
                                int64_t timestamp = image.getTimestamp();
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
                                    cameraChannel.stopCapture();
                                    loop = false;
                                }
                            }
                        }
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

#include <id3Devices/id3HighResTimer.h>
#include <id3Devices/helpers/id3DevicesCpp.h>
#include <format>
#include <iostream>

//#define LOOP_MODE_THREAD

bool waitForDevice(uint32_t delayMs) {
    auto timeout = id3HighResTimer_GetTickCountMS() + delayMs;
    for(;;) {
        id3HighResTimer_WaitMS(20);
#ifndef LOOP_MODE_THREAD
        DeviceManager::doEvent(); // only if message loop is set to none
#endif
        auto deviceCount = DeviceManager::getDeviceCount();
        if (deviceCount >= 1) {
            return true;
        }
        if (id3HighResTimer_GetTickCountMS() > timeout) {
            break;
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

    int sdk_err;
    CHECK_ID3_ERROR(sdk_err, DeviceManager::checkLicense("c:/ProgramData/id3/id3FaceToolkit_v9.lic"));
    CHECK_ID3_ERROR(sdk_err, DeviceManager::start());
    CHECK_ID3_ERROR(sdk_err, DeviceManager::loadPlugin("id3DevicesWebcam"));

    std::cout << "Wait for device" << std::endl;
    auto result = waitForDevice(2000);
    if (result) {
        DeviceInfoList devInfoList;
        devInfoList.getList();
        printf("found %d FingerPrint device(s)\n",devInfoList.count());

        Camera cameraChannel;
        DeviceInfo device;
        devInfoList.get(0, device);
        sdk_err = cameraChannel.openDevice(device.deviceId());
        if (sdk_err == 0) {
            bool loop = true;
            while (loop) {
#ifndef LOOP_MODE_THREAD
                DeviceManager::doEvent();
#endif
                id3DevicesDeviceState state = cameraChannel.state();
                switch (state) {
                    case id3DevicesDeviceState_NoDevice:
                    case id3DevicesDeviceState_DeviceError:
                        std::cout << "lost device" << std::endl;
                        loop = false;
                        break;
                    case id3DevicesDeviceState_DeviceReady: {
                        CHECK_ID3_ERROR(sdk_err, cameraChannel.startCapture());
                        break;
                    }
                    case id3DevicesDeviceState_CaptureInProgress: {
#ifdef LOOP_MODE_THREAD
                        auto available = cameraChannel.waitForCapture(100, false);
#else
                        auto available = cameraChannel.waitForCapture(100, true);
#endif
                        if (available) {
                            CapturedImage image;
                            available = cameraChannel.getCurrentFrame(image);
                            if (available) {
                                int frameNumber = image.frameCount();
                                int64_t timestamp = image.timestamp();
                                printf("frameNumber %6d: timestamp %lld\n", frameNumber, timestamp);

                                int width  = image.width();
                                int height = image.height();
                                int stride = image.stride();
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
    }

    DeviceManager::stop();
    DeviceManager::dispose();
    return sdk_err;
}

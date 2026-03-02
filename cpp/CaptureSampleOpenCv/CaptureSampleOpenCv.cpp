#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <id3Devices/id3HighResTimer.h>
#include <id3DevicesCppWrapper/id3DevicesDeviceManager.hpp>
#include <id3DevicesCppWrapper/id3DevicesCamera.hpp>
#include <id3DevicesCppWrapper/id3DevicesLicense.hpp>

using namespace id3DevicesCppWrapper;

#define CV2_WINDOW "Display window"

using namespace cv;
int main() {
    try {
        DeviceManager::initialize();
        DeviceManager::configure(id3DevicesMessageLoopMode_InternalMessageLoop);
        DevicesLicense::checkLicense("c:/ProgramData/id3/id3FaceToolkit_v9.lic");
        DeviceManager::start();
        DeviceManager::loadPlugin("id3DevicesWebcam");
    }
    catch (DevicesException &e) {
        std::cout << e.what() << std::endl;
    }

    id3HighResTimer_WaitMS(1000);

    try {
        int deviceCount = DeviceManager::getDeviceCount();

        auto deviceList = DeviceManager::getDeviceInfoList();
        deviceCount = deviceList.getCount();

        Camera cameraChannel{};
        if(deviceCount > 0) {
            auto device = deviceList.get(0);
            int deviceId = device.getDeviceId();
            cameraChannel.openDevice(deviceId);
        }

        CaptureImage image;

        namedWindow(CV2_WINDOW, WINDOW_GUI_EXPANDED | WINDOW_KEEPRATIO);

        auto deviceInfo = cameraChannel.getDeviceInfo();
        auto deviceModelName = deviceInfo.getModel();

        int last_frame_number = -1;
        bool configured = false;
        bool loop = true;
        while(loop) {
            auto state = cameraChannel.getDeviceState();
            switch (state) {
                case id3DevicesDeviceState_DeviceReady: {
                    cameraChannel.startCapture();
                    break;
                }
                case id3DevicesDeviceState_CaptureInProgress: {
                    if (!configured) {
                        configured = true;
                        auto videoFormatList = cameraChannel.getVideoFormatList();
                        int videoFormat = videoFormatList.findNearestVideoFormat(1280, 720, 20);
                        if (videoFormat >= 0) {
                            cameraChannel.setVideoFormat(videoFormat);
                        }
                    }
                    bool available = cameraChannel.waitForCapture(100, false);
                    if (available) {
                        // Code pour la realsense -> use stream
                        available = cameraChannel.getCurrentFrame(image);
                        if (available) {
                            int frameNumber = image.getFrameCount();
                            int64_t timestamp = image.getTimestamp();
                            last_frame_number = frameNumber;
                            printf("frameNumber %6d: timestamp %lld\n", frameNumber, timestamp);

                            int width  = image.getWidth();
                            int height = image.getHeight();
                            int stride = image.getStride();
                            void *pixels = image.getPixels();
                            cv::Mat cvpicture = cv::Mat(height, width, CV_8UC3, pixels);
                            imshow(CV2_WINDOW, cvpicture);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            if (loop) {
                int keycode = waitKey(1);
                if (keycode == 27) {
                    cameraChannel.stopCapture();
                    loop = false;
                }
                else if (getWindowProperty(CV2_WINDOW, WND_PROP_VISIBLE) == 0 ) {
                    if(cameraChannel.getIsCapturing()) {
                        cameraChannel.stopCapture();
                    }
                    loop = false;
                }
            }
        }
    }
    catch (DevicesException &e) {
        std::cout << e.what() << std::endl;
    }

    destroyAllWindows();
    DeviceManager::dispose();
    return 0;
}

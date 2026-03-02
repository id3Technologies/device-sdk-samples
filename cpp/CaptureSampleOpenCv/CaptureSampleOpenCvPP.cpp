#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <id3Devices/id3HighResTimer.h>
#include <id3DevicesCppWrapper/id3DevicesDeviceManager.hpp>
#include <id3DevicesCppWrapper/id3DevicesCamera.hpp>
#include <id3DevicesCppWrapper/id3DevicesLicense.hpp>
#include <atomic>
#include <iostream>

using namespace id3DevicesCppWrapper;

#define CV2_WINDOW "Display window"

using namespace cv;

bool cameraChannelConfigured;
void cameraPlugAndPlayAddedCallback(void *context, int deviceId) {
    try {
        auto cameraChannel = static_cast<Camera *>(context);
        if (cameraChannel != nullptr) {
            cameraChannel->openDevice(deviceId);
        }
    }
    catch (DevicesException &e) {
        std::cout << e.what() << std::endl;
    }
}

void cameraPlugAndPlayRemovedCallback(void *context, int deviceId) {
    try {
        auto cameraChannel = static_cast<Camera *>(context);
        cameraChannelConfigured = false;
        auto black = Mat::zeros(720, 1280, CV_8UC3);
        imshow(CV2_WINDOW, black);
    }
    catch (DevicesException &e) {
        std::cout << e.what() << std::endl;
    }
}

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

    try {
        Camera cameraChannel;
        cameraChannel.setDeviceAddedCallback(cameraPlugAndPlayAddedCallback, &cameraChannel);
        cameraChannel.setDeviceRemovedCallback(cameraPlugAndPlayRemovedCallback, &cameraChannel);

        CaptureImage image;
        namedWindow(CV2_WINDOW, WINDOW_GUI_EXPANDED | WINDOW_KEEPRATIO);

        bool loop = true;
        while(loop) {
            auto state = cameraChannel.getDeviceState();
            switch (state) {
                case id3DevicesDeviceState_DeviceReady: {
                    cameraChannel.startCapture();
                    break;
                }
                case id3DevicesDeviceState_CaptureInProgress: {
                    if (!cameraChannelConfigured) {
                        cameraChannelConfigured = true;
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

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <id3Devices/id3HighResTimer.h>
#include <id3Devices/helpers/id3DevicesCpp.h>
#include <atomic>

#define CV2_WINDOW "Display window"

using namespace cv;

bool cameraChannelConfigured;
void cameraPlugAndPlayAddedCallback(void *context, int deviceId) {
    auto cameraChannel = (Camera *)context;
    if (cameraChannel != nullptr) {
        cameraChannel->openDevice(deviceId);
    }
}

void cameraPlugAndPlayRemovedCallback(void *context, int deviceId) {
    auto cameraChannel = (Camera *)context;
    cameraChannelConfigured = false;
    auto black = Mat::zeros(720, 1280, CV_8UC3);
    imshow(CV2_WINDOW, black);
}

int main() {
	DeviceManager::initialize();
    DeviceManager::configure(id3DevicesMessageLoopMode_InternalMessageLoop);

    int sdk_err;
    CHECK_ID3_ERROR(sdk_err, DeviceManager::checkLicense("c:/ProgramData/id3/id3FaceToolkit_v9.lic"));
    CHECK_ID3_ERROR(sdk_err, DeviceManager::start());
    CHECK_ID3_ERROR(sdk_err, DeviceManager::loadPlugin("id3DevicesWebcam"));
    Camera cameraChannel;
    cameraChannel.setDeviceAddedCallback(cameraPlugAndPlayAddedCallback, &cameraChannel);
    cameraChannel.setDeviceRemovedCallback(cameraPlugAndPlayRemovedCallback, &cameraChannel);

    CapturedImage image;
    namedWindow(CV2_WINDOW, WINDOW_GUI_EXPANDED | WINDOW_KEEPRATIO);

    bool loop = true;
    while(loop) {
        id3DevicesDeviceState state = cameraChannel.state();
        switch (state) {
            case id3DevicesDeviceState_DeviceReady: {
                CHECK_ID3_ERROR(sdk_err, cameraChannel.startCapture());
                break;
            }
            case id3DevicesDeviceState_CaptureInProgress: {
                if (!cameraChannelConfigured) {
                    cameraChannelConfigured = true;
                    int videoFormat = cameraChannel.findNearestVideoFormat(1280, 720, 20);
                    if (videoFormat >= 0) {
                        cameraChannel.setVideoFormat(videoFormat);
                    }
                }
                bool available = cameraChannel.waitForCapture(100, false);
                if (available) {
                    // Code pour la realsense -> use stream
                    available = cameraChannel.getCurrentFrame(image);
                    if (available) {
                        int frameNumber = image.frameCount();
                        int64_t timestamp = image.timestamp();
                        printf("frameNumber %6d: timestamp %lld\n", frameNumber, timestamp);

                        int width  = image.width();
                        int height = image.height();
                        int stride = image.stride();
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
                CHECK_ID3_ERROR(sdk_err, cameraChannel.stopCapture());
                loop = false;
            }
            else if (getWindowProperty(CV2_WINDOW, WND_PROP_VISIBLE) == 0 ) {
                if(cameraChannel.isCapturing()) {
                    CHECK_ID3_ERROR(sdk_err, cameraChannel.stopCapture());
                }
                loop = false;
            }
        }
    }

    destroyAllWindows();
    DeviceManager::dispose();
    return sdk_err;
}

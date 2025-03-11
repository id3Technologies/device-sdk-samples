import time
import cv2 as cv
import numpy
import id3devices as cap

def sample_capture():
    try:
        # license
        cap.DevicesLicense.check_license("id3Devices.lic")
        # Initialise device manager and load webcam plugin
        manager = cap.DeviceManager()
        manager.configure(cap.MessageLoopMode.THREAD)
        manager.load_plugin("id3DevicesWebcam")
        manager.start()
    except cap.DevicesException as e:
        print(e)

    time.sleep(0.5)
    count = manager.device_count
    print(f"Found {count} device(s)")

    device_list = manager.get_device_info_list()
    for i in range(count):
        dev = device_list.get(i)
        print(f" [{dev.device_id}]:'{dev.name}'")
    
    cv2_window = "Display window"
    cv.namedWindow(cv2_window, cv.WINDOW_GUI_EXPANDED or cv.WINDOW_KEEPRATIO)

    # Build a capture channel
    capture = cap.Camera()
    if count > 0:
        device = device_list.get(0)
        device_id = device.device_id
        print(f"open device id:{device_id}, name:'{device.name}'")
        capture.open_device(device_id)

    image = cap.CaptureImage()
    configured = False
    loop = True
    while loop:
        try:
            #manager.do_event()
            status = capture.device_state
            if status in {cap.DeviceState.NO_DEVICE, cap.DeviceState.DEVICE_ERROR}:
                print("lost device")
                loop = False
            elif status == cap.DeviceState.DEVICE_READY:
                capture.start_capture(None)
            elif status == cap.DeviceState.CAPTURE_IN_PROGRESS:
                if not configured:
                    configured = True
                    resolution = capture.video_format_list.find_nearest_video_format(1280, 720, 20)
                    if resolution != None:
                        capture.set_parameter_value_int("VideoFormat", resolution)
                available = capture.wait_for_capture(100, False)
                if available:
                    capture.get_current_frame(image)
                    frameNumber = image.frame_count
                    timestamp = image.timestamp
                    print(f"frameNumber {frameNumber:6} timestamp {timestamp:16}")
                    picture = image.to_numpy()
                    cv.imshow(cv2_window, picture)
            if loop:
                if cv.waitKey(1) == 27:
                    capture.stop_capture()
                    loop = False
                elif cv.getWindowProperty(cv2_window, cv.WND_PROP_VISIBLE) == 0:
                    capture.stop_capture()
                    loop = False
        except cap.DevicesException as e:
            print(e)
            break

    manager.stop()
    print("end")

if __name__ == '__main__':
    sample_capture()

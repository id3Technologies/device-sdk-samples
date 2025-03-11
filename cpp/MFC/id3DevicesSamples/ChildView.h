// ChildView.h : interface of the CChildView class

#pragma once
#include <id3DevicesLib.h>
#include <id3Devices/helpers/id3DevicesCpp.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

#ifdef USE_FACE_SDK
#include <id3FaceLib.h>
#endif

class CChildView : public CWnd
{
public:
    CChildView();
    virtual ~CChildView();

    void Initialize(CStatusBar *statusBar);
    void Dispose();
    void DeviceAddedHandler(int32_t device_id);
    void DeviceRemovedHandler(int32_t device_id);
    void DeviceStatusChangedHandler(id3DevicesDeviceCaptureStatus eType);
    void ImageCapturedHandler();

protected:
#ifdef USE_FACE_SDK
    static std::atomic<bool> isDetectingPortrait;
    static std::atomic<bool> isDetectingPresentationAttack;
    static std::condition_variable cv;
    static std::mutex cv_mutex;
    static std::atomic<bool> stopBackgroundThread;

    void ConvertDeviceImageToFaceImage(ID3_DEVICES_CAPTURE_IMAGE hSrcPicture, ID3_FACE_IMAGE hFaceImage);
    bool DetectPortrait(ID3_DEVICES_CAPTURE_IMAGE hSrcPicture, id3FaceRectangle & bounds);
    void StartBackgroundThread();
    void StopBackgroundThread();
    void BackgroundDetectPortrait();
    void BackgroundDetectPresentationAttack();
    void HandleFaceDetection();
    void RenderFaceRectangle(CPaintDC& dc, const RECT& destRect, int img_width);
#endif
    void CleanupResources();
    void SelectVideoFormat(int width, int height, int fps);

    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPaint();
    afx_msg void OnClose();
    void RenderCapturedImage(CPaintDC& dc);
    void RenderImageToDevice(CPaintDC& dc);

    std::string SeparatePascalCaseWords(const std::string & input);

    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

    DECLARE_MESSAGE_MAP()

private:
    CStatusBar *m_statusBar{};
    CImage m_image;
    Camera m_camera;
    CapturedImage m_currentPicture;
    std::vector<uint8_t> m_pixels{};
#ifdef USE_FACE_SDK
    CRect m_facialRect;
    ID3_FACE_PORTRAIT_PROCESSOR m_hPortraitProcessor;
    ID3_FACE_PORTRAIT m_hPortrait;
    COLORREF m_color;
#endif
};
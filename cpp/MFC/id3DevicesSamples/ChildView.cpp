// ChildView.cpp : CChildView class implementation

#include "pch.h"
#include "framework.h"
#include "id3DevicesSamples.h"
#include "ChildView.h"
#include <id3DevicesCppWrapper/id3DevicesDeviceManager.hpp>
#include <id3DevicesCppWrapper/id3DevicesLicense.hpp>

#ifdef USE_FACE_SDK
#include <id3FaceCppWrapper/id3FaceLicense.hpp>
#include <id3FaceCppWrapper/id3FaceLibrary.hpp>
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef USE_FACE_SDK
// Atomic lock and condition variable to manage background threads for image processing
std::atomic<bool> CChildView::isDetectingPortrait(false);
std::atomic<bool> CChildView::isDetectingPresentationAttack(false);
std::condition_variable CChildView::cv;
std::mutex CChildView::cv_mutex;
std::atomic<bool> CChildView::stopBackgroundThread(false);
#endif

const std::string LICENSE_PATH = "C:\\ProgramData\\id3\\id3Devices.lic";
const std::string MODELS_PATH = "C:\\ProgramData\\id3\\models";

static void check_sdkerr(int err, const char *fmt, ...)
{
    if (err != 0) {
        exit(1);
    }
}

// C Camera Event Handler
static void CaptureManager_DeviceAddedHandler(void *context, int deviceId)
{
    static_cast<CChildView*>(context)->DeviceAddedHandler(deviceId);
}

static void CaptureManager_DeviceRemovedHandler(void* context, int device_id)
{
    static_cast<CChildView*>(context)->DeviceRemovedHandler(device_id);
}

static void Camera_DeviceStatusChangedHandler(void* context, id3DevicesDeviceCaptureStatus eType, int parameter)
{
    static_cast<CChildView*>(context)->DeviceStatusChangedHandler(eType);
}

static void Camera_ImageCapturedHandler(void* context)
{
    static_cast<CChildView*>(context)->ImageCapturedHandler();
}

// CChildView
BEGIN_MESSAGE_MAP(CChildView, CWnd)
    ON_WM_ERASEBKGND()
    ON_WM_PAINT()
    ON_WM_CLOSE()
END_MESSAGE_MAP()

CChildView::CChildView()
{
}

CChildView::~CChildView()
{
    CleanupResources();
}

void CChildView::Initialize(CStatusBar* statusBar)
{
    m_statusBar = statusBar;

    try
    {
        DevicesLicense::checkLicense(LICENSE_PATH.c_str());

        // Initialize the device manager
        DeviceManager::initialize();

        // Configure the device manager
        DeviceManager::configure(id3DevicesMessageLoopMode_ApplicationMessageLoop);

        // Configure device manager callbacks
        DeviceManager::setDeviceAddedCallback(CaptureManager_DeviceAddedHandler, this);

        // Load camera plugin
        DeviceManager::loadPlugin("id3DevicesWebcam");

        // Start the device manager.
        DeviceManager::start();

        // Initialize the camera
        m_camera.initialize();
        m_camera.setDeviceRemovedCallback(CaptureManager_DeviceRemovedHandler, this);
        m_camera.setDeviceStatusChangedCallback(Camera_DeviceStatusChangedHandler, this);
        m_camera.setCaptureCallback(Camera_ImageCapturedHandler, this);

#ifdef USE_FACE_SDK
        // Check Face SDK license
        FaceLicense::checkLicense(LICENSE_PATH.c_str());

        // Load face models
        FaceLibrary::loadModel(MODELS_PATH.c_str(), id3FaceModel_FaceDetector4B, id3FaceProcessingUnit_Cpu);
        FaceLibrary::loadModel(MODELS_PATH.c_str(), id3FaceModel_FaceAttributesClassifier2A, id3FaceProcessingUnit_Cpu);
        FaceLibrary::loadModel(MODELS_PATH.c_str(), id3FaceModel_FaceEncoder9B, id3FaceProcessingUnit_Cpu);
        FaceLibrary::loadModel(MODELS_PATH.c_str(), id3FaceModel_FaceLandmarksEstimator2A, id3FaceProcessingUnit_Cpu);
        FaceLibrary::loadModel(MODELS_PATH.c_str(), id3FaceModel_FaceOcclusionDetector2A, id3FaceProcessingUnit_Cpu);
        FaceLibrary::loadModel(MODELS_PATH.c_str(), id3FaceModel_FacePoseEstimator1A, id3FaceProcessingUnit_Cpu);
        FaceLibrary::loadModel(MODELS_PATH.c_str(), id3FaceModel_FaceColorBasedPad3A, id3FaceProcessingUnit_Cpu);

        // Initialize portrait capture
        m_portraitProcessor.setThreadCount(4);
        m_portraitProcessor.setFaceDetectionImageSize(256);

        // Start the background thread for portrait detection
        StartBackgroundThread();
#endif
    }
    catch (DevicesException &e)
    {
        exit(1);
    }
}

void CChildView::Dispose()
{
}

void CChildView::DeviceAddedHandler(int32_t device_id)
{
    try
    {
        auto deviceInfo = DeviceManager::getDeviceInfo(device_id);
        auto camera_name = deviceInfo.getName();
        std::string msg = "Found camera " + camera_name;
        m_statusBar->SetPaneText(0, msg.c_str());

        // Auto select camera
        if (m_camera.getDeviceState() == id3DevicesDeviceState_NoDevice)
        {
            m_camera.openDevice(device_id);
        }
    }
    catch (DevicesException& e)
    {
        m_statusBar->SetPaneText(1, e.what());
    }
}

void CChildView::DeviceRemovedHandler(int32_t device_id)
{
    try
    {
        auto deviceInfo = DeviceManager::getDeviceInfo(device_id);
        auto camera_name = deviceInfo.getName();
        std::string msg = "Lost camera " + camera_name;
        m_statusBar->SetPaneText(0, msg.c_str());
    }
    catch (DevicesException& e)
    {
        m_statusBar->SetPaneText(1, e.what());
    }
}

void CChildView::DeviceStatusChangedHandler(id3DevicesDeviceCaptureStatus eType)
{
    try
    {
        auto deviceInfo = m_camera.getDeviceInfo();
        std::string camera_name = deviceInfo.getName();
        if (eType == id3DevicesDeviceCaptureStatus_DeviceReady)
        {
            SelectVideoFormat(1920, 1080, 30);
            m_camera.startCapture();
            m_statusBar->SetPaneText(0, ("Start camera " + camera_name).c_str());
            Invalidate();
        }
        else if (eType == id3DevicesDeviceCaptureStatus_DeviceError)
        {
            m_statusBar->SetPaneText(0, "Camera error");
        }
    }
    catch (DevicesException& e)
    {
        m_statusBar->SetPaneText(1, e.what());
    }
}

void CChildView::ImageCapturedHandler()
{
    try
    {
        if (m_camera.getCurrentFrame(m_currentPicture))
        {
            int height = m_currentPicture.getHeight();
            if (height > 0)
            {
#ifdef USE_FACE_SDK
                HandleFaceDetection();
#endif
                Invalidate(FALSE); // FALSE to avoid erasing the background
            }
        }
    }
    catch (DevicesException& e)
    {
        m_statusBar->SetPaneText(1, e.what());
    }
}

#ifdef USE_FACE_SDK
void CChildView::HandleFaceDetection()
{
    if (!isDetectingPortrait.exchange(true))
    {
        cv.notify_one();
    }
    if (!isDetectingPresentationAttack.exchange(true))
    {
        cv.notify_one();
    }
}

void CChildView::ConvertDeviceImageToFaceImage(CaptureImage& srcPicture, Image& faceImage)
{
    try
    {
        int width = srcPicture.getWidth();
        int height = srcPicture.getHeight();
        unsigned char* pixels_src = (unsigned char*)srcPicture.getPixels();
        faceImage.set(width, height, id3FacePixelFormat_Bgr24Bits, pixels_src);
    }
    catch (DevicesException& e)
    {
    }
}

bool CChildView::DetectPortrait(CaptureImage& srcPicture, id3FaceCppWrapper::Rectangle &bounds)
{
    try
    {
        bool result = false;
        int sdk_err = id3FaceError_Base;

        // create face image
        Image face_image;
        ConvertDeviceImageToFaceImage(srcPicture, face_image);

        m_portraitProcessor.updatePortrait(m_portrait, face_image);

        id3FacePortraitStatus status;
        id3FacePortrait_GetStatus(m_portrait, &status);

        switch (status)
        {
        case id3FacePortraitStatus::id3FacePortraitStatus_Created:
            break;

        case id3FacePortraitStatus::id3FacePortraitStatus_Updated:
        {
            auto detectedFaceItem = m_portrait.getTrackedFace();
            auto bounds = detectedFaceItem.getBounds();

            result = true;
            break;
        }
        }
        return result;
    }
    catch (DevicesException& e)
    {
    }
    return false;
}

void CChildView::BackgroundDetectPortrait()
{
    while (!stopBackgroundThread)
    {
        std::unique_lock<std::mutex> lk(cv_mutex);
        cv.wait(lk, [] { return isDetectingPortrait || stopBackgroundThread; });

        if (stopBackgroundThread)
        {
            break;
        }

        // Perform portrait detection logic here
        id3FaceCppWrapper::Rectangle bounds;
        if (DetectPortrait(m_currentPicture, bounds))
        {
            m_facialRect.left = bounds.getTopLeft().X;
            m_facialRect.top = bounds.getTopLeft().Y;
            m_facialRect.right = bounds.getBottomRight().X;
            m_facialRect.bottom = bounds.getBottomRight().Y;
        }
        Sleep(10);

        isDetectingPortrait = false;
    }
}

void CChildView::BackgroundDetectPresentationAttack()
{
    while (!stopBackgroundThread)
    {
        std::unique_lock<std::mutex> lk(cv_mutex);
        cv.wait(lk, [] { return isDetectingPresentationAttack || stopBackgroundThread; });

        if (stopBackgroundThread)
        {
            break;
        }

        // Perform presentation attack detection logic here
        auto status = m_portrait.getStatus();
        if (status == id3FacePortraitStatus::id3FacePortraitStatus_Updated)
        {
            auto padStatus = m_portrait.getPadStatus();
            if (padStatus == id3FacePadStatus::id3FacePadStatus_Unknown)
            {
                m_portraitProcessor.detectOcclusions(m_portrait);
                m_portraitProcessor.estimatePhotographicQuality(m_portrait);
                m_portraitProcessor.estimateFaceAttributes(m_portrait);
                m_portraitProcessor.detectPresentationAttack(m_portrait);
            }
        }

        Sleep(100);
        isDetectingPresentationAttack = false;
    }
}

void CChildView::StartBackgroundThread()
{
    std::thread(&CChildView::BackgroundDetectPortrait, this).detach();
    std::thread(&CChildView::BackgroundDetectPresentationAttack, this).detach();
}

void CChildView::StopBackgroundThread()
{
    stopBackgroundThread = true;
    cv.notify_all();
}
#endif

void CChildView::CleanupResources()
{
#ifdef USE_FACE_SDK
    StopBackgroundThread();
    //id3FacePortraitProcessor_Dispose(&m_hPortraitProcessor);
    //id3FacePortrait_Dispose(&m_hPortrait);
#endif
    try
    {
        m_camera.stopCapture();
        Sleep(100);
        DeviceManager::stop();
        DeviceManager::dispose();
    }
    catch (DevicesException& e)
    {
        m_statusBar->SetPaneText(1, e.what());
    }
}

void CChildView::OnClose()
{
    CleanupResources();
    CWnd::OnClose();
}

void CChildView::SelectVideoFormat(int width, int height, int fps)
{
    try
    {
        auto videoFormatList = m_camera.getVideoFormatList();
        auto result = videoFormatList.findNearestVideoFormat(width, height, fps);
        if (result >= 0) {
            m_camera.setVideoFormat(result);
        }
    }
    catch (DevicesException& e)
    {
        m_statusBar->SetPaneText(1, e.what());
    }
}

BOOL CChildView::OnEraseBkgnd(CDC* pDC)
{
    RECT rect{};
    GetClientRect(&rect);

    CBrush backBrush(RGB(0, 0, 0));
    pDC->FillRect(&rect, &backBrush);

    return TRUE;
}

void CChildView::OnPaint()
{
    CPaintDC dc(this); // device context for painting
    if (m_currentPicture)
    {
        RenderCapturedImage(dc);
    }
}

void CChildView::RenderCapturedImage(CPaintDC& dc)
{
    try
    {
        int height = m_currentPicture.getHeight();
        if (height <= 0) return;

        int width = m_currentPicture.getWidth();
        if (m_image.IsNull()) {
            m_image.Create(width, -height, 24); // -height because need top-down DIB
        }
        int stride_src = m_currentPicture.getStride();
        auto pixels_src = (uint8_t*)m_currentPicture.getPixels();

        int stride_dst = m_image.GetPitch();
        if (stride_dst == stride_src)
        {
            void* pixels_dst = m_image.GetBits();
            memcpy(pixels_dst, pixels_src, stride_src * height);
        }
        else
        {
            for (int y = 0; ++y < height; )
            {
                void* pixels_dst = m_image.GetPixelAddress(0, y);
                memcpy(pixels_dst, pixels_src, stride_src);
                pixels_src += stride_src;
            }
        }
    }
    catch (DevicesException& e)
    {
        m_statusBar->SetPaneText(1, e.what());
    }
    RenderImageToDevice(dc);
}

void CChildView::RenderImageToDevice(CPaintDC& dc)
{
    if (m_image.IsNull()) return;
    try
    {
        RECT rect{};
        GetClientRect(&rect);

        int img_width = m_image.GetWidth();
        int img_height = m_image.GetHeight();
        RECT destRect{};
        float r1 = static_cast<float>(img_width) / img_height;
        float r2 = static_cast<float>(rect.right) / rect.bottom;
        int w = rect.right;
        int h = rect.bottom;
        if (r1 > r2)
        {
            w = rect.right;
            h = static_cast<int>(w / r1);
        }
        else if (r1 < r2)
        {
            h = rect.bottom;
            w = static_cast<int>(r1 * h);
        }
        destRect.left = rect.left + (rect.right - w) / 2;
        destRect.top = rect.top + (rect.bottom - h) / 2;
        destRect.right = destRect.left + w;
        destRect.bottom = destRect.top + h;

        dc.SetStretchBltMode(HALFTONE);
        m_image.StretchBlt(dc.GetSafeHdc(), destRect, SRCCOPY);

#ifdef USE_FACE_SDK
        auto status = m_portrait.getStatus();
        if (status == id3FacePortraitStatus_Updated)
            RenderFaceRectangle(dc, destRect, img_width);
#endif
    }
    catch (DevicesException& e)
    {
        m_statusBar->SetPaneText(1, e.what());
    }
}

#ifdef USE_FACE_SDK
void CChildView::RenderFaceRectangle(CPaintDC& dc, const RECT& destRect, int img_width)
{
    CRect boundsRect = m_facialRect;
    double ratio = static_cast<double>(destRect.right - destRect.left) / img_width;
    boundsRect.left = static_cast<int>(boundsRect.left * ratio);
    boundsRect.top = static_cast<int>(boundsRect.top * ratio);
    boundsRect.right = static_cast<int>(boundsRect.right * ratio);
    boundsRect.bottom = static_cast<int>(boundsRect.bottom * ratio);
    boundsRect.OffsetRect(destRect.left, destRect.top);

    // display face rectangle
    auto padStatus = m_portrait.getPadStatus();
    switch (padStatus)
    {
    case id3FacePadStatus::id3FacePadStatus_Unknown:
        m_color = RGB(0, 51, 221);
        break;
    case id3FacePadStatus::id3FacePadStatus_Bonafide:
        m_color = RGB(0, 221, 52);
        break;
    default:
        m_color = RGB(221, 52, 0);
        break;
    }

    CPen pen(PS_SOLID, 3, m_color);
    dc.SelectStockObject(NULL_BRUSH);
    dc.SelectObject(pen);
    dc.RoundRect(boundsRect, CPoint(15, 15));

    // display instruction
    auto instruction = m_portrait.getInstruction();
    if (instruction != id3FacePortraitInstruction_None)
    {
        dc.TextOutA(boundsRect.left, boundsRect.bottom, SeparatePascalCaseWords(id3Face_GetPortraitInstructionString(instruction)).c_str());
    }
}
#endif

std::string CChildView::SeparatePascalCaseWords(const std::string& input)
{
    std::string result;
    for (size_t i = 0; i < input.length(); ++i)
    {
        if (isupper(input[i]) && i != 0)
        {
            result += ' ';
        }
        result += input[i];
    }
    return result;
}

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CWnd::PreCreateWindow(cs))
        return FALSE;

    cs.dwExStyle |= WS_EX_CLIENTEDGE;
    cs.style &= ~WS_BORDER;
    cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS,
        ::LoadCursor(nullptr, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr);

    return TRUE;
}
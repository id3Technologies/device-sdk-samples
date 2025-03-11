// ChildView.cpp : CChildView class implementation

#include "pch.h"
#include "framework.h"
#include "id3DevicesSamples.h"
#include "ChildView.h"
#include <id3Devices/helpers/DeviceHelper.h>

#ifdef USE_FACE_SDK
#include <id3FaceLib.h>
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

CChildView::CChildView() : m_camera(false)
{
}

CChildView::~CChildView()
{
    CleanupResources();
}

void CChildView::Initialize(CStatusBar* statusBar)
{
    m_statusBar = statusBar;

    int sdk_err = DeviceManager::checkLicense(LICENSE_PATH.c_str());
    check_sdkerr(sdk_err, "id3DevicesLicense_CheckLicense");

    // Initialize the device manager
    sdk_err = DeviceManager::initialize();
    check_sdkerr(sdk_err, "id3DevicesDeviceManager_Initialize");

    // Configure the device manager
    sdk_err = DeviceManager::configure(id3DevicesMessageLoopMode_ApplicationMessageLoop);
    check_sdkerr(sdk_err, "id3DevicesDeviceManager_Initialize");

    // Configure device manager callbacks
    sdk_err = DeviceManager::setDeviceAddedCallback(CaptureManager_DeviceAddedHandler, this);
    check_sdkerr(sdk_err, "SetDeviceAddedCallback");

    // Load camera plugin
    sdk_err = DeviceManager::loadPlugin("id3DevicesWebcam");
    check_sdkerr(sdk_err, "id3DevicesDeviceManager_LoadPlugin");

    // Start the device manager.
    sdk_err = DeviceManager::start();

    // Initialize the camera
    sdk_err = m_camera.initialize();
    check_sdkerr(sdk_err, "id3DevicesCamera_Initialize");

    sdk_err = m_camera.setDeviceRemovedCallback(CaptureManager_DeviceRemovedHandler, this);
    check_sdkerr(sdk_err, "SetDeviceRemovedCallback");

    sdk_err = m_camera.setDeviceStatusChangedCallback(Camera_DeviceStatusChangedHandler, this);
    check_sdkerr(sdk_err, "SetDeviceStatusChangedCallback");

    sdk_err = m_camera.setCaptureCallback(Camera_ImageCapturedHandler, this);
    check_sdkerr(sdk_err, "SetCaptureCallback");

#ifdef USE_FACE_SDK
    // Check Face SDK license
    sdk_err = id3FaceLicense_CheckLicense(LICENSE_PATH.c_str(), nullptr);
    check_sdkerr(sdk_err, "id3FaceLibrary_CheckLicense");

    // Load face models
    sdk_err = id3FaceLibrary_LoadModel(MODELS_PATH.c_str(), id3FaceModel_FaceDetector4B, id3FaceProcessingUnit_Cpu);
    check_sdkerr(sdk_err, "id3FaceLibrary_LoadModel - FaceDetector4B");

    sdk_err = id3FaceLibrary_LoadModel(MODELS_PATH.c_str(), id3FaceModel_FaceAttributesClassifier2A, id3FaceProcessingUnit_Cpu);
    check_sdkerr(sdk_err, "id3FaceLibrary_LoadModel - FaceAttributesClassifier2A");

    sdk_err = id3FaceLibrary_LoadModel(MODELS_PATH.c_str(), id3FaceModel_FaceEncoder9B, id3FaceProcessingUnit_Cpu);
    check_sdkerr(sdk_err, "id3FaceLibrary_LoadModel - FaceEncoder9B");

    sdk_err = id3FaceLibrary_LoadModel(MODELS_PATH.c_str(), id3FaceModel_FaceLandmarksEstimator2A, id3FaceProcessingUnit_Cpu);
    check_sdkerr(sdk_err, "id3FaceLibrary_LoadModel - FaceLandmarksEstimator2A");

    sdk_err = id3FaceLibrary_LoadModel(MODELS_PATH.c_str(), id3FaceModel_FaceOcclusionDetector2A, id3FaceProcessingUnit_Cpu);
    check_sdkerr(sdk_err, "id3FaceLibrary_LoadModel - FaceOcclusionDetector2A");

    sdk_err = id3FaceLibrary_LoadModel(MODELS_PATH.c_str(), id3FaceModel_FacePoseEstimator1A, id3FaceProcessingUnit_Cpu);
    check_sdkerr(sdk_err, "id3FaceLibrary_LoadModel - FacePoseEstimator1A");

    sdk_err = id3FaceLibrary_LoadModel(MODELS_PATH.c_str(), id3FaceModel_FaceColorBasedPad3ARC1, id3FaceProcessingUnit_Cpu);
    check_sdkerr(sdk_err, "id3FaceLibrary_LoadModel - FaceColorBasedPad3ARC1");

    // Initialize portrait capture
    sdk_err = id3FacePortraitProcessor_Initialize(&m_hPortraitProcessor);
    check_sdkerr(sdk_err, "id3FacePortraitProcessor_Initialize");

    id3FacePortraitProcessor_SetThreadCount(m_hPortraitProcessor, 4);
    id3FacePortraitProcessor_SetFaceDetectionImageSize(m_hPortraitProcessor, 256);

    sdk_err = id3FacePortrait_Initialize(&m_hPortrait);
    check_sdkerr(sdk_err, "id3FacePortrait_Initialize");

    // Start the background thread for portrait detection
    StartBackgroundThread();
#endif
}

void CChildView::Dispose()
{
}

void CChildView::DeviceAddedHandler(int32_t device_id)
{
    DeviceInfo deviceInfo(device_id);
    auto camera_name = deviceInfo.getName();
    std::string msg = "Found camera " + camera_name;
    m_statusBar->SetPaneText(0, msg.c_str());

    // Auto select camera
    if (m_camera.state() == id3DevicesDeviceState_NoDevice)
    {
        m_camera.openDevice(device_id);
    }
}

void CChildView::DeviceRemovedHandler(int32_t device_id)
{
    DeviceInfo deviceInfo(device_id);
    auto camera_name = deviceInfo.getName();
    std::string msg = "Lost camera " + camera_name;
    m_statusBar->SetPaneText(0, msg.c_str());
}

void CChildView::DeviceStatusChangedHandler(id3DevicesDeviceCaptureStatus eType)
{
    int currentDeviceId = m_camera.deviceId();
    DeviceInfo deviceInfo(currentDeviceId);
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

void CChildView::ImageCapturedHandler()
{
    if (m_camera.getCurrentFrame(m_currentPicture))
    {
        int height = m_currentPicture.height();
        if (height > 0)
        {
#ifdef USE_FACE_SDK
            HandleFaceDetection();
#endif
            Invalidate(FALSE); // FALSE to avoid erasing the background
        }
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

void CChildView::ConvertDeviceImageToFaceImage(ID3_DEVICES_CAPTURE_IMAGE hSrcPicture, ID3_FACE_IMAGE hFaceImage)
{
    int width = 0;
    int height = 0;
    unsigned char *pixels_src{};
    id3DevicesCaptureImage_GetWidth(hSrcPicture, &width);
    id3DevicesCaptureImage_GetHeight(hSrcPicture, &height);
    id3DevicesCaptureImage_GetPixels(hSrcPicture, (void**)&pixels_src);
    id3FaceImage_Set(hFaceImage, width, height, id3FacePixelFormat_Bgr24Bits, pixels_src);
}

bool CChildView::DetectPortrait(ID3_DEVICES_CAPTURE_IMAGE hSrcPicture, id3FaceRectangle &bounds)
{
    bool result = false;
    int sdk_err = id3FaceError_Base;

    ID3_FACE_IMAGE face_image{};

    // create face image
    sdk_err = id3FaceImage_Initialize(&face_image);
    ConvertDeviceImageToFaceImage(hSrcPicture, face_image);

    id3FacePortraitProcessor_UpdatePortrait(m_hPortraitProcessor, m_hPortrait, face_image);

    id3FacePortraitStatus status;
    id3FacePortrait_GetStatus(m_hPortrait, &status);

    switch (status)
    {
    case id3FacePortraitStatus::id3FacePortraitStatus_Created:
        break;

    case id3FacePortraitStatus::id3FacePortraitStatus_Updated:
    {
        ID3_TRACKED_FACE hDetectedFaceItem{};
        id3TrackedFace_Initialize(&hDetectedFaceItem);
        id3FacePortrait_GetTrackedFace(m_hPortrait, hDetectedFaceItem);
        sdk_err = id3TrackedFace_GetBounds(hDetectedFaceItem, &bounds);
        id3TrackedFace_Dispose(&hDetectedFaceItem);

        result = true;
        break;
    }
    }

    id3FaceImage_Dispose(&face_image);
    return result;
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
        id3FaceRectangle bounds{};
        if (DetectPortrait(m_hCurrentPicture, bounds))
        {
            m_facialRect.left = bounds.TopLeft.X;
            m_facialRect.top = bounds.TopLeft.Y;
            m_facialRect.right = bounds.BottomRight.X;
            m_facialRect.bottom = bounds.BottomRight.Y;
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
        id3FacePortraitStatus status;
        id3FacePortrait_GetStatus(m_hPortrait, &status);

        if (status == id3FacePortraitStatus::id3FacePortraitStatus_Updated)
        {
            id3FacePadStatus padStatus;
            id3FacePortrait_GetPadStatus(m_hPortrait, &padStatus);

            if (padStatus == id3FacePadStatus::id3FacePadStatus_Unknown)
            {
                id3FacePortraitProcessor_DetectOcclusions(m_hPortraitProcessor, m_hPortrait);
                id3FacePortraitProcessor_EstimatePhotographicQuality(m_hPortraitProcessor, m_hPortrait);
                id3FacePortraitProcessor_EstimateFaceAttributes(m_hPortraitProcessor, m_hPortrait);
                id3FacePortraitProcessor_DetectPresentationAttack(m_hPortraitProcessor, m_hPortrait, false);
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
    id3FacePortraitProcessor_Dispose(&m_hPortraitProcessor);
    id3FacePortrait_Dispose(&m_hPortrait);
#endif
    m_camera.stopCapture();
    Sleep(100);
    DeviceManager::stop();
    DeviceManager::dispose();
}

void CChildView::OnClose()
{
    CleanupResources();
    CWnd::OnClose();
}

void CChildView::SelectVideoFormat(int width, int height, int fps)
{
    auto result = m_camera.findNearestVideoFormat(width, height, fps);
    if (result >= 0) {
        m_camera.setVideoFormat(result);
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
    int height = m_currentPicture.height();
    if (height <= 0) return;

    int width = m_currentPicture.width();
    if (m_image.IsNull()) {
        m_image.Create(width, -height, 24); // -height because need top-down DIB
    }
    int stride_src = m_currentPicture.stride();
    auto pixels_src = (uint8_t *)m_currentPicture.getPixels();

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

    RenderImageToDevice(dc);
}

void CChildView::RenderImageToDevice(CPaintDC& dc)
{
    if (m_image.IsNull()) return;

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
    id3FacePortraitStatus status;
    id3FacePortrait_GetStatus(m_hPortrait, &status);
    if (status == id3FacePortraitStatus_Updated)
        RenderFaceRectangle(dc, destRect, img_width);
#endif
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
    id3FacePadStatus padStatus;
    id3FacePortrait_GetPadStatus(m_hPortrait, &padStatus);
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
    id3FacePortraitInstruction instruction;
    id3FacePortrait_GetInstruction(m_hPortrait, &instruction);
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
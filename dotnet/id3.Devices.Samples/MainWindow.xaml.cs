using System;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Collections.ObjectModel;

namespace id3.Capture.Samples
{
    using id3.Devices;
    using System.IO;

    /// <summary>
    /// Logique d'interaction pour MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private static readonly string appDataDir = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData, Environment.SpecialFolderOption.Create), @"id3");
        private static readonly string licensePath = $"{appDataDir}\\id3Devices.lic";
    
        // simple lock for controls 
        volatile int _lockControls;
        string _status;
        CaptureImage _currentPicture;
        Camera _camera;
        ObservableCollection<DeviceInfo> CameraList { get; } = new ObservableCollection<DeviceInfo>();

        // current live bitmap
        WriteableBitmap _currentBitmap;

        string Status
        {
            get { return _status; }
            set
            {
                if (_status != value)
                {
                    _status = value;
                    lblStatus.Text = value;
                }
            }
        }

        public MainWindow()
        {
            InitializeComponent();
            DataContext = this;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            _currentPicture = new CaptureImage();

            // id3Camera initialisation
            try
            {
                DevicesLicense.CheckLicense(licensePath);
                DeviceManager.Initialize();
                DeviceManager.Configure(MessageLoopMode.ApplicationMessageLoop);
                DeviceManager.DeviceAdded += DeviceManager_DeviceAdded;
                DeviceManager.DeviceRemoved += DeviceManager_DeviceRemoved;
                //DeviceManager.AddPluginPath(Path.GetFullPath(@"..\..\..\..\sdk\devices"));
                DeviceManager.LoadPlugin("id3DevicesWebcam");

                // Device manager startup
                DeviceManager.Start();

                // instantiate a new camera.
                _camera = new Camera();
                _camera.DeviceStatusChanged += Camera_DeviceStatusChanged;
                _camera.Capture += Camera_ImageCaptured;

                comboBoxCameraList.ItemsSource = CameraList;
            }
            catch (Exception ex)
            {
                MessageBox.Show(string.Format("CaptureManager exception: {0}", ex.Message));
                Environment.Exit(-1);
            }

            Status = "Initialization done. camera plug & play process started.";
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            _camera.StopCapture();
            _camera.Dispose();
            DeviceManager.Dispose();
            _currentPicture.Dispose();
        }

        #region Capture events
        /// <summary>
        /// Occurs when a device is added.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void DeviceManager_DeviceAdded(object sender, PlugAndPlayCallbackEventArgs e)
        {
            DeviceInfo deviceInfo = DeviceManager.GetDeviceInfo(e.DeviceId);
            if (deviceInfo != null)
            {
                if (deviceInfo.DeviceType == DeviceType.Camera)
                {
                    Status = "Found camera " + deviceInfo.Name;
                    CameraList.Add(deviceInfo);

                    if (CameraList.Count == 1)
                        _camera.OpenDevice(deviceInfo.DeviceId);
                }
            }
        }

        /// <summary>
        /// Occurs when a device is removed.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void DeviceManager_DeviceRemoved(object sender, PlugAndPlayCallbackEventArgs e)
        {
            DeviceInfo deviceInfo = DeviceManager.GetDeviceInfo(e.DeviceId);

            if (deviceInfo != null)
            {
                if (deviceInfo.DeviceType == DeviceType.Camera)
                {
                    Status = "Lost camera " + deviceInfo.Name;
                    CameraList.Remove(deviceInfo);
                }
            }
        }

        /// <summary>
        /// Occurs when the status of the camera changes.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Camera_DeviceStatusChanged(object sender, DeviceCaptureStatusChangedCallbackEventArgs e)
        {
            if (e.Status == DeviceCaptureStatus.DeviceReady)
            {
                SelectFormat(_camera, 1920, 1080, 30);

                _lockControls++;
                comboBoxCameraList.Text = _camera.DeviceInfo.Name;
                _lockControls--;

                _camera.StartCapture();
            }
            else if (e.Status == DeviceCaptureStatus.CaptureStarted)
            {
                _currentBitmap = null;
            }
            else if (e.Status == DeviceCaptureStatus.DeviceError)
            {
                Status = "";
            }
        }

        /// <summary>
        /// Occurs when an image is captured.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Camera_ImageCaptured(object sender, EventArgs e)
        {
            // Get current camera for our camera slot and process only if good camera and page is visible
            var measurement_data = new MeasurementData();
            _camera.GetMeasurementData(measurement_data);
            var cam_fps = measurement_data.GetDataFloat("AcquireFrameRate");
            var cam_ctr = measurement_data.GetDataInt("AcquireFrameCount");
            string fps_str = $"F ={cam_fps:00.00} fps {cam_ctr:00000000}";
            Status = fps_str;

            // Get current camera frame
            bool available = _camera.GetCurrentFrame(_currentPicture);
            if (available && (_currentPicture.Height > 0))
            {
                ToBitmapSource(_currentPicture, ref _currentBitmap);

                // Draw current frame
                imagePreview.BeginInit();
                imagePreview.Source = _currentBitmap;
                imagePreview.EndInit();
            }
        }

        /// <summary>
        /// Selects a video format.
        /// </summary>
        /// <param name="camera"></param>
        /// <param name="width"></param>
        /// <param name="height"></param>
        /// <param name="fps"></param>
        private void SelectFormat(Camera camera, int width, int height, int fps)
        {
            VideoFormatList res_list = camera.VideoFormatList;

            if (res_list != null)
            {
                int auto_select = res_list.FindNearestVideoFormat(width, height, fps + 1);
                if (auto_select >= 0)
                {
                    int actual_res = camera.VideoFormat;
                    if (actual_res != res_list[auto_select].Value)
                    {
                        camera.VideoFormat = res_list[auto_select].Value;
                    }
                }
            }
        }
        #endregion

        private void CameraList_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_lockControls > 0)
                return;

            DeviceInfo deviceInfo = e.AddedItems[0] as DeviceInfo;

            // open the selected camera
            _camera.OpenDevice(deviceInfo.DeviceId);
        }

        /// <summary>
        /// Creates a BitmapSource from a Capturemage.
        /// </summary>
        /// <param name="image"></param>
        /// <param name="wbm"></param>
        /// <returns></returns>
        private BitmapSource ToBitmapSource(CaptureImage image, ref WriteableBitmap wbm)
        {
            if (image.PixelDepth != 1)
            {
                int w = image.Width;
                int h = image.Height;
                int bufferSize = image.Height * image.Stride;

                if (wbm == null)
                    wbm = new WriteableBitmap(w, h, 96, 96, PixelFormats.Bgr24, null);

                if ((h <= wbm.Height) && (w <= wbm.Width))
                {
                    IntPtr pixelsPtr = image.GetPixels();
                    wbm.WritePixels(new Int32Rect(0, 0, w, h), pixelsPtr, bufferSize, image.Stride);
                }
            }
            return wbm;
        }
    }
}

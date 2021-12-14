using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Microsoft.Graphics.Canvas;
using Windows.UI;
using System.IO.MemoryMappedFiles;
using System.Threading;
using System.Numerics;

namespace Overlay
{
    public sealed partial class Overlay : Page
    {
        public Overlay()
        {
            this.InitializeComponent();
        }

        private void overlayCanvas_Loaded(object sender, RoutedEventArgs e)
        {
            CanvasDevice device = CanvasDevice.GetSharedDevice();
            overlayCanvas.SwapChain = new CanvasSwapChain(device, (float)Window.Current.CoreWindow.Bounds.Width, (float)Window.Current.CoreWindow.Bounds.Height, 96);

            Thread renderThread = new Thread(new ParameterizedThreadStart(RenderThread));
            renderThread.Start(overlayCanvas.SwapChain);
        }

        private void RenderThread(object parameter)
        {
            CanvasSwapChain swapChain = (CanvasSwapChain)parameter;

            using (MemoryMappedFile memoryMappedFile = MemoryMappedFile.CreateOrOpen("SharedMemory", 90024, MemoryMappedFileAccess.ReadWriteExecute))
            {
                using (MemoryMappedViewAccessor viewAccessor = memoryMappedFile.CreateViewAccessor())
                {
                    using (Mutex mutex = new Mutex(false, "Mutex"))
                    {
                        while (true)
                        {
                            try
                            {
                                mutex.WaitOne();

                               
                                int Esize = 3;
                                EllipseM[] EArray = new EllipseM[Esize];
                                viewAccessor.ReadArray<EllipseM>(0, EArray, 0, Esize);

                                Vector2 center;
                                

                                using (CanvasDrawingSession ds = swapChain.CreateDrawingSession(Colors.Transparent))
                                {
                                    for (int i = 0; i < EArray.Length; i++)
                                    {
                                        center.X = EArray[i].x;
                                        center.Y = EArray[i].y;
                                        ds.DrawEllipse(center, EArray[i].radiusX, EArray[i].radiusY, Colors.AliceBlue);
                                    }
                                    
                                }

                                swapChain.Present();
                            }
                            finally
                            {
                                mutex.ReleaseMutex();
                            }
                        }
                    }
                }
            }
        }

        struct EllipseM
        {
            public float x;
            public float y;
            public float radiusX;
            public float radiusY;
        };
    }
}

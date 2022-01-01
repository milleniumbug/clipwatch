using System;
using System.Runtime.InteropServices;
using System.Text;

namespace ClipwatchSharp
{
    public class ClipboardWatcher : IDisposable
    {
        private readonly ClipwatchHandle handle;

        private static void FreeContext(IntPtr rawHandle)
        {
            var gcHandle = GCHandle.FromIntPtr(rawHandle);
            gcHandle.Free();
        }

        private static NativeFunctions.UserDataDeleteCallback FreeContextDelegate = FreeContext;

        private static unsafe void Callback(byte* buffer, UIntPtr length, IntPtr userData)
        {
            var gcHandle = GCHandle.FromIntPtr(userData);
            var context = (ClipboardWatcher)gcHandle.Target;
            context.ClipboardChanged?.Invoke(context, Encoding.UTF8.GetString(buffer, (int)length.ToUInt32()));
        }

        private static unsafe NativeFunctions.UserCallback CallbackDelegate = Callback;
        
        public event EventHandler<string> ClipboardChanged;

        public unsafe ClipboardWatcher(int pollingIntervalInMs = 500)
        {
            var gcHandle = GCHandle.Alloc(this);
            this.handle = NativeFunctions.Clipwatch_Init(pollingIntervalInMs, CallbackDelegate, GCHandle.ToIntPtr(gcHandle), FreeContextDelegate);
        }

        public void Start()
        {
            NativeFunctions.Clipwatch_Start(this.handle);
        }
        
        public void Stop()
        {
            NativeFunctions.Clipwatch_Stop(this.handle);
        }

        public void Dispose()
        {
            this.handle.Dispose();
        }
    }
}
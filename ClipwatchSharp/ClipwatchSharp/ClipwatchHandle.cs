using System;
using Microsoft.Win32.SafeHandles;

namespace ClipwatchSharp
{
    internal class ClipwatchHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        public ClipwatchHandle(IntPtr handle) : base(true)
        {
            SetHandle(handle);
        }
        
        public ClipwatchHandle() : base(true)
        {
            
        }

        protected override bool ReleaseHandle()
        {
            NativeFunctions.Clipwatch_Release(this.handle);
            return true;
        }
    }
}
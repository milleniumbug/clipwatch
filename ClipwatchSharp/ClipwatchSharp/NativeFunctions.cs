using System;
using System.Runtime.InteropServices;

namespace ClipwatchSharp
{
    internal class NativeFunctions
    {
        public const string LibraryName = "clipwatch";
        
        public unsafe delegate void UserCallback(byte* buffer, UIntPtr length, IntPtr userData);
        
        public unsafe delegate void UserDataDeleteCallback(IntPtr userData);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern ClipwatchHandle Clipwatch_Init(
            int pollingIntervalInMs,
            UserCallback clipboardEventHandler,
            IntPtr userData,
            UserDataDeleteCallback userDataDeleter);
        
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void Clipwatch_Release(
            IntPtr handle);
        
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void Clipwatch_Start(
            ClipwatchHandle handle);
        
        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        public static extern void Clipwatch_Stop(
            ClipwatchHandle handle);
    }
}
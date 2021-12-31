mod clipwatch {
    use std::thread;
    use std::thread::JoinHandle;
    use std::boxed::Box;
    use filedescriptor::Pipe;
    use x11::xlib::{XOpenDisplay, XDefaultRootWindow, XInternAtom, XConnectionNumber, XFlush};
    use x11::xfixes::XFixesSelectSelectionInput;
    use std::ffi::{CString};

    pub struct ClipboardWatcher {
        thread_join_handle: JoinHandle<()>,
        pipe: Pipe
    }

    #[no_mangle]
    pub extern "C" fn clipwatch_init() -> *mut ClipboardWatcher {
        if let Ok(pipe) = Pipe::new() {
            let handle = Box::<ClipboardWatcher>::new(ClipboardWatcher{
                pipe: pipe,
                thread_join_handle: thread::spawn(move || {
                    unsafe {
                        let display = XOpenDisplay(std::ptr::null());
                        if !display.is_null() {
                            return;
                        }
                        let root_window = XDefaultRootWindow(display);
                        let clipboard_text = CString::new("CLIPBOARD").unwrap();
                        let clipboard_text_ptr = clipboard_text.as_ptr();
                        let clipboard_atom = XInternAtom(display, clipboard_text_ptr, 0);

                        XFixesSelectSelectionInput(display, root_window, clipboard_atom, 1 << 0);

                        XFlush(display);

                        let x11_fd = XConnectionNumber(display);

                        
                    }
                })
            });

            return Box::into_raw(handle);
        }
        else {
            return std::ptr::null_mut();
        }
    }

    #[no_mangle]
    pub extern "C" fn clipwatch_release(raw_handle: *mut ClipboardWatcher) {
        unsafe {
            let handle = Box::from_raw(raw_handle);

        }
    }

    #[no_mangle]
    pub extern "C" fn clipwatch_stop(raw_handle: *mut ClipboardWatcher) {

    }

    #[no_mangle]
    pub extern "C" fn clipwatch_start(raw_handle: *mut ClipboardWatcher) {

    }
}
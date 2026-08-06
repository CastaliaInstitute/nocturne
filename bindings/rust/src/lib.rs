use libc::{c_char, c_uint};

#[repr(C)]
pub struct NocturneSdkVersionInfo {
    major: c_uint,
    minor: c_uint,
    patch: c_uint,
    semver: *const c_char,
}

#[no_mangle]
pub extern "C" fn nocturne_sdk_version_info() -> NocturneSdkVersionInfo {
    NocturneSdkVersionInfo {
        major: 1,
        minor: 0,
        patch: 0,
        semver: b"1.0.0\0".as_ptr() as *const c_char,
    }
}

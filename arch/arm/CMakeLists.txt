include_directories(
    common/include
    common/include/usb
    common/${ARMVER}/include
    ../${ARCH}/include
)

add_subdirectory(common/source)
add_subdirectory(common/${ARMVER}/source)
add_subdirectory(common/source/usb)
add_subdirectory(common/userspace)
add_subdirectory(common/${ARMVER}/userspace)

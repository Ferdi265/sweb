include_directories(
    common/include
    common/include/usb
    ../${ARCH}/include
    ../${ARCH}/../common/include
)

add_subdirectory(common/source)
add_subdirectory(common/source/usb)
add_subdirectory(common/userspace)

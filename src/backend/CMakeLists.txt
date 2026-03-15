add_library(qmonstatek_backend STATIC
    # Protocol
    protocol/crc16.cpp
    protocol/rpc_frame.cpp

    # Transport
    transport/serial_transport.cpp
    transport/device_discovery.cpp

    # Device
    device/m1_device.cpp
    device/screen_buffer.cpp
    device/screen_image_provider.cpp
    device/log_model.cpp

    # Updater
    updater/github_checker.cpp
    updater/firmware_updater.cpp
    updater/esp32_updater.cpp
    updater/self_updater.cpp
    updater/dfu_flasher.cpp
    updater/swd_recovery.cpp

    # File manager
    filemanager/remote_filesystem.cpp
    filemanager/file_model.cpp
)

target_include_directories(qmonstatek_backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(qmonstatek_backend PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::SerialPort
    Qt6::Network
)

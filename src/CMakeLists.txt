# This is the main binary. List your C files here.
add_executable(can can.c
    )


# Modify the below lines to enable/disable output over UART/USB.
# These are mutually exclusive, only set one.
pico_enable_stdio_uart(can 1)
pico_enable_stdio_usb(can 0)

# Add the libraries to the build
target_link_libraries(can PRIVATE
  pico_stdlib
  pico_multicore
  pico_async_context_freertos
  can2040
  FreeRTOS-Kernel-Heap4
)

# The CYW43 is the Wifi/Bluetooth module. If the board is set to pico_w, this
# variable will be set and the wireless libraries added.
if(PICO_CYW43_SUPPORTED)
    # For led support on pico_w
    target_link_libraries(can PRIVATE
        pico_cyw43_arch_none
    )
endif()

# Another pico macro which generates the uf2 file in addition to the elf
pico_add_extra_outputs(can)

find_program(OPENOCD openocd)
find_program(PICOTOOL picotool)

# add_custom_target(flash ${PICOTOOL} load -f $<TARGET_FILE:hello_freertos> DEPENDS hello_freertos)

# add_custom_target(openocd_flash ${OPENOCD} -s ${OPENOCD_PATH} -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c 'adapter speed 5000; program "$<TARGET_FILE:hello_freertos>" verify reset exit' DEPENDS hello_freertos)

add_custom_target(flash ${PICOTOOL} load -f $<TARGET_FILE:can> DEPENDS can)

add_custom_target(flash_can
  COMMAND ${OPENOCD} -s ${OPENOCD_PATH}
          -f interface/cmsis-dap.cfg
          -f target/rp2040.cfg
          -c "adapter speed 5000"
          -c "init"
          -c "reset halt"
          -c "program {$<TARGET_FILE:can>} verify reset exit"
  DEPENDS can
  USES_TERMINAL
)

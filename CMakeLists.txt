if(CONFIG_NANOCBOR)
    zephyr_include_directories(
        ${ZEPHYR_NANOCBOR_MODULE_DIR}/include
    )

    zephyr_library()
    zephyr_library_sources(
        ${ZEPHYR_NANOCBOR_MODULE_DIR}/src/decoder.c
        ${ZEPHYR_NANOCBOR_MODULE_DIR}/src/encoder.c
    )
endif()

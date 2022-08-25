add_library(vgmcore INTERFACE)

target_sources(vgmcore INTERFACE
    blip_buf.c
    nesapu.c
    vgm.c
)

target_include_directories(vgmcore INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)
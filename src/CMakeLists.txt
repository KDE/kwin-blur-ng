# SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(client)
if (ONLY_CLIENT_BUILD)
    return()
endif()

add_subdirectory(scanner)

add_library(
    kwin_effect_blur_ng
    blur.cpp
    main.cpp
    wayland/blurinterface.cpp
    shaders.qrc
)

kconfig_add_kcfg_files(kwin_effect_blur_ng
    blurconfig.kcfgc
)

ecm_add_qtwayland_server_protocol_kde(kwin_effect_blur_ng
    PROTOCOL ${CMAKE_SOURCE_DIR}/protocols/mbition-blur-v1.xml
    BASENAME mbition-blur-v1
)

ecm_qt_declare_logging_category(kwin_effect_blur_ng
    HEADER kwinblurng_debug.h
    IDENTIFIER KWIN_BLUR
    CATEGORY_NAME io.mbition.kwinblurng
)

target_link_libraries(kwin_effect_blur_ng PRIVATE
    KWin::kwin
    Wayland::Server
    KF6::ConfigGui
    KDecoration2::KDecoration
)
install(
    TARGETS
        kwin_effect_blur_ng

    DESTINATION
        ${KDE_INSTALL_PLUGINDIR}/kwin/effects/plugins/
)

ecm_add_qml_module(KWinBlurNG
                  GENERATE_PLUGIN_SOURCE
                  URI "org.kde.blurng"
                  VERSION 1.0
                  SOURCES blurbehind.cpp blurclient.cpp blurbehindmask.cpp shm.cpp)

target_link_libraries(KWinBlurNG PRIVATE Qt::Quick Qt::WaylandClient Qt::GuiPrivate)

qt6_generate_wayland_protocol_client_sources(KWinBlurNG FILES
    ${CMAKE_SOURCE_DIR}/protocols/mbition-blur-v1.xml
    ${Wayland_DATADIR}/wayland.xml
)
ecm_qt_declare_logging_category(KWinBlurNG
    HEADER kwinblurngclientlogging.h
    IDENTIFIER KWINBLURNG_CLIENT
    CATEGORY_NAME kwinblurng.client
    DEFAULT_SEVERITY Info
)

ecm_finalize_qml_module(KWinBlurNG DESTINATION ${KDE_INSTALL_QMLDIR})

# SPDX-FileCopyrightText: Copyright (c) 2025 MBition GmbH.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(kwin-blur-ng VERSION 0.1 LANGUAGES C CXX)

set(QT_MIN_VERSION "6.5.2")
set(KF_MIN_VERSION "6.0")
set(KDE_COMPILERSETTINGS_LEVEL "6.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

include(FeatureSummary)
include(GenerateExportHeader)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)
include(ECMQmlModule)
find_package(Qt6WaylandClient ${REQUIRED_QT_VERSION} CONFIG REQUIRED)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)

find_package(QtWaylandScanner REQUIRED)
set_package_properties(QtWaylandScanner PROPERTIES
    TYPE REQUIRED
    PURPOSE "Required for building this effect"
)

find_package(Wayland 1.20 REQUIRED COMPONENTS Client)
set_package_properties(Wayland PROPERTIES
    TYPE REQUIRED
    PURPOSE "Required for building this effect"
)

find_package(KDecoration2 6.0 CONFIG)
set_package_properties(KDecoration2 PROPERTIES
    TYPE REQUIRED
    PURPOSE "Required for server side decoration support"
)

add_definitions(-DQT_NO_KEYWORDS)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    Config
    WindowSystem
)
find_package(epoxy 1.3 REQUIRED)
find_package(KWin REQUIRED)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

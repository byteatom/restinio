QT -= gui
QT += network

CONFIG += c++11 console
CONFIG -= app_bundle

CONFIG += c++14

DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += ASIO_STANDALONE ASIO_HAS_STD_CHRONO HTTP_PARSER_STRICT=0 FMT_HEADER_ONLY RAPIDJSON_HAS_STDSTRING RAPIDJSON_HAS_CXX11_RVALUE_REFS

SOURCES += \
    main.cpp \
    ../../nodejs/http_parser/http_parser.c

INCLUDEPATH += ../.. ../../asio/include ../../fmt/include ../../nodejs/http_parser


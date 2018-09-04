CONFIG(debug, debug|release) {
    CONFIG-=release
    BUILD_PATH = $${PWD}/build/debug
} else {
    CONFIG*=release
    BUILD_PATH = $${PWD}/build/release
    QMAKE_CXXFLAGS += -g
}
CONFIG*=c++11
CONFIG = $$unique(CONFIG)

DESTDIR = $${BUILD_PATH}

GENERATED_FILES_DIR=$${BUILD_PATH}/.obj
OBJECTS_DIR = $${GENERATED_FILES_DIR}/o
MOC_DIR = $${GENERATED_FILES_DIR}/moc
LIBS *= -L$${BUILD_PATH}

qt_add_library(ducktor_core STATIC
    core/AppController.cpp core/AppController.h
    core/AppSettings.cpp core/AppSettings.h
    core/Languages.cpp core/Languages.h
    core/Paths.cpp core/Paths.h
    engines/EngineProcess.cpp engines/EngineProcess.h
    engines/FrameParser.cpp engines/FrameParser.h
    audio/AudioDsp.cpp audio/AudioDsp.h
    audio/AudioPlayer.cpp audio/AudioPlayer.h
    audio/AudioRecorder.cpp audio/AudioRecorder.h
    models/ModelCatalog.cpp models/ModelCatalog.h
    models/ModelListModel.cpp models/ModelListModel.h
    models/ModelManager.cpp models/ModelManager.h
    models/VoiceTableModel.cpp models/VoiceTableModel.h)

target_include_directories(ducktor_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ducktor_core PUBLIC Qt6::Core Qt6::Gui Qt6::Network Qt6::Multimedia)

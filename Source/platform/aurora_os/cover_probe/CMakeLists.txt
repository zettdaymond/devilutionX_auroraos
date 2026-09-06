# Зонд размера плитки обложки — отдельный бинарь (только Aurora-сборка):
# спрашивает Silica Theme и печатает ответ. Qt в игру не линкуется,
# зонд живёт секунду в собственном процессе. Каталог bin после установки
# materializer Авроры перекладывает рядом с основным бинарём, поэтому
# приложение ищет его относительно /proc/self/exe.
find_package(Qt5 COMPONENTS Core Gui Qml REQUIRED)

add_executable(devilutionx-coverprobe cover_probe.cpp)
target_link_libraries(devilutionx-coverprobe PRIVATE Qt5::Core Qt5::Gui Qt5::Qml)

install(TARGETS devilutionx-coverprobe RUNTIME DESTINATION bin)

# Зонд размера плитки обложки — отдельный бинарь (только Aurora-сборка):
# спрашивает Silica Theme и печатает ответ. Qt в игру не линкуется,
# зонд живёт секунду в собственном процессе. Каталог bin после установки
# materializer Авроры перекладывает рядом с основным бинарём, поэтому
# приложение ищет его относительно /proc/self/exe.
find_package(Qt5 COMPONENTS Core Gui Qml REQUIRED)

add_executable(org.diasurgical.devilutionx.coverprobe cover_probe.cpp)
target_link_libraries(org.diasurgical.devilutionx.coverprobe PRIVATE Qt5::Core Qt5::Gui Qt5::Qml)

# Стрип: рантайм символы не читает, а бинарь пакета опрятнее.
target_link_options(org.diasurgical.devilutionx.coverprobe PRIVATE -s)

install(TARGETS org.diasurgical.devilutionx.coverprobe RUNTIME DESTINATION bin)

# Идемпотентное применение патча zoe-no-install.patch.
# Вызывается из FetchContent PATCH_COMMAND как `cmake -P` — единственный
# интерпретатор, доступный во всех средах порта: SDK mb2/BT (Linux),
# Qt Creator/MSYS2 (команды идут через cmd.exe, где нет sh).
#
# argv: [0]=cmake [1]=-P [2]=этот скрипт [3]=путь до патча.
# Рабочий каталог наследуется от шага populate (_deps/zoe-src).

set(patchFile "${CMAKE_ARGV3}")

execute_process(
    COMMAND git apply --ignore-whitespace "${patchFile}"
    RESULT_VARIABLE result)

if(result EQUAL 0)
    return()
endif()

# Не применился? Единственная допустимая причина — он уже лежит
# (окружения делят build-дерево, штампы переживают mtime-скачки).
# Reverse-check это подтверждает; любая иная ситуация — ошибка.
execute_process(
    COMMAND git apply --ignore-whitespace --reverse --check "${patchFile}"
    RESULT_VARIABLE reverse)

if(reverse EQUAL 0)
    message(STATUS "zoe: патч уже применён, пропускаем")
    return()
endif()

message(FATAL_ERROR
    "zoe: патч не применился (apply rc=${result}, reverse-check rc=${reverse}): ${patchFile}")

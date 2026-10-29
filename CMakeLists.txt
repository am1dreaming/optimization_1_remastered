# Две вариации запуска и последующие проверки:
#    Встроенный cmake clion (возможны ошибки)
#    Доп. установленный но создается отдельная папка bulid в этом случае стоит учитывать путь
cmake_minimum_required(VERSION 3.10)
project(rebus C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# По умолчанию - оптимизированная сборка (замеры без -O2 не имеют смысла)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    # /utf-8 - исходники в UTF-8 (кириллица в выводе)
    add_compile_options(/utf-8 /W3)
else()
    add_compile_options(-Wall -Wextra)
endif()

# Профилирование через gprof: cmake -DREBUS_PROFILE=ON (только gcc, Linux/MinGW)
option(REBUS_PROFILE "Build with -pg for gprof" OFF)
# полный сет версий (Clion может стереть в случае обновления cmake)
set(VERSIONS
    v0_baseline # полный перебор, сумма считается заново на каждом листе
    v1_coeffs # весовые коэффициенты букв (машинно-независимая)
    v2_incremental # инкрементальная сумма (машинно-независимая)
    v3_bounds # отсечение по границам суммы (алгоритмическая)
    v3_order # порядок букв по |коэффициенту| (эвристика)
    v4_columns # поразрядный перебор с переносом (алгоритмическая)
    v5_forced # цифра результата вычисляется (алгоритмическая)
    v6_bitmask # битовая маска + ctz (машинно-зависимая)
)

foreach(v ${VERSIONS})
    add_executable(${v} ${v}.c)
    if(REBUS_PROFILE AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${v} PRIVATE -pg)
        target_link_options(${v} PRIVATE -pg)
    endif()
endforeach()

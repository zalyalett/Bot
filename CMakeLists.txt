cmake_minimum_required(VERSION 3.12)
project(MusicBot)

# Используем C++17
set(CMAKE_CXX_STANDARD 17)

# Ищем библиотеки
find_package(PkgConfig REQUIRED)
pkg_check_modules(TGBOT REQUIRED tgbot-cpp)
pkg_check_modules(CURL REQUIRED libcurl)

# Создаем программу файлов
add_executable(music_bot 
    main.cpp 
    MusicLibrary.cpp
)

# Подключаем библиотеки к программе
target_link_libraries(music_bot ${TGBOT_LIBRARIES} ${CURL_LIBRARIES})
target_include_directories(music_bot PRIVATE ${TGBOT_INCLUDE_DIRS})
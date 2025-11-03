cmake_minimum_required(VERSION 3.10)
project(SoftwareRenderer)

# Configurar C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontrar SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Encontrar GLM
find_package(glm REQUIRED)

# Archivos fuente
set(SOURCES
    main.cpp
)

# Crear el ejecutable
add_executable(renderer ${SOURCES})

# Enlazar bibliotecas
target_link_libraries(renderer ${SDL2_LIBRARIES} glm::glm)

# En Windows, copiar las DLLs necesarias
if(WIN32)
    add_custom_command(TARGET renderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL2::SDL2>
        $<TARGET_FILE_DIR:renderer>
    )
endif()
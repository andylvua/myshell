file(GLOB EXTERNAL_FILES src/external/*)

set(EXTERNAL ${EXTERNAL_PROGRAMS})
set(EXTERNAL_BIN_PATH ${CMAKE_BINARY_DIR}/msh/bin/external)

foreach (EXTERNAL_FILE ${EXTERNAL_FILES})
    if (IS_DIRECTORY ${EXTERNAL_FILE})
        add_subdirectory(${EXTERNAL_FILE})

        get_directory_property(EXTERNAL_PROGRAM_TARGET DIRECTORY ${EXTERNAL_FILE} DEFINITION PROJECT_NAME)

        if (NOT EXTERNAL_PROGRAM_TARGET)
            message(WARNING "No PROJECT_NAME defined in ${EXTERNAL_FILE}'s CMakeLists.txt")
            continue()
        endif()

        message(STATUS "Adding external program ${EXTERNAL_PROGRAM_TARGET}")

        set(EXTERNAL ${EXTERNAL} ${EXTERNAL_PROGRAM_TARGET})
        set_target_properties(${EXTERNAL_PROGRAM_TARGET}
                PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${EXTERNAL_BIN_PATH})
    endif ()
endforeach ()

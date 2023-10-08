file(GLOB EXTERNAL_FILES src/external/*)

set(EXTERNAL ${EXTERNAL_PROGRAMS})
set(EXTERNAL_BIN_PATH ${CMAKE_BINARY_DIR}/external)

foreach (EXTERNAL_FILE ${EXTERNAL_FILES})
    if (IS_DIRECTORY ${EXTERNAL_FILE})
        message(STATUS "Adding external program ${EXTERNAL_FILE}")
        add_subdirectory(${EXTERNAL_FILE})
        get_filename_component(EXTERNAL_PROGRAM_TARGET ${EXTERNAL_FILE} NAME)
        set(EXTERNAL ${EXTERNAL} ${EXTERNAL_PROGRAM_TARGET})
        set_target_properties(${EXTERNAL_PROGRAM_TARGET}
                PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${EXTERNAL_BIN_PATH})
    endif ()
endforeach ()

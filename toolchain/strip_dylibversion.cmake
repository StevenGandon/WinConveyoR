find_program(INSTALL_NAME_TOOL install_name_tool REQUIRED)

set(_renames "")

file(GLOB _libs
    "${LIB_DIR}/*.dylib"
)

foreach(_lib ${_libs})
    get_filename_component(_name "${_lib}" NAME)

    string(REGEX REPLACE "(\\.[0-9]+(\\.[0-9]+)*)\\.dylib$" "\\1" _base "${_name}" )

    if(NOT _base STREQUAL _name)
        get_filename_component(_dir "${_lib}" DIRECTORY)
        set(_new "${_dir}/${_base}")

        message(STATUS "Stripping dylib version: ${_name} -> ${_base}")

        execute_process(
            COMMAND
                "${INSTALL_NAME_TOOL}"
                -id
                "${_base}"
                "${_lib}"
            RESULT_VARIABLE _result
        )

        if(NOT _result EQUAL 0)
            message(FATAL_ERROR
                "install_name_tool failed for ${_lib}"
            )
        endif()

        file(RENAME "${_lib}" "${_new}")

        list(APPEND _renames "${_name}:${_base}")
    endif()
endforeach()

file(GLOB _all_bins
    "${BIN_DIR}/*"
    "${LIB_DIR}/*.dylib"
)

foreach(_bin ${_all_bins})

    if(IS_DIRECTORY "${_bin}")
        continue()
    endif()

    foreach(_pair ${_renames})
        string(REPLACE ":" ";" _pair_list "${_pair}")

        list(GET _pair_list 0 _old)
        list(GET _pair_list 1 _new)

        execute_process(
            COMMAND
                "${INSTALL_NAME_TOOL}"
                -change
                "${_old}"
                "${_new}"
                "${_bin}"
            RESULT_VARIABLE _result
            ERROR_QUIET
        )

        get_filename_component(_bin_dir "${_bin}" DIRECTORY)

        set(_old_full "${_bin_dir}/${_old}")
        set(_new_full "${_bin_dir}/${_new}")

        execute_process(
            COMMAND
                "${INSTALL_NAME_TOOL}"
                -change
                "${_old_full}"
                "${_new_full}"
                "${_bin}"
            RESULT_VARIABLE _result
            ERROR_QUIET
        )
    endforeach()
endforeach()

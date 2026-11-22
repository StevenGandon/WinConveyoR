file(GLOB _libs "${LIB_DIR}/*.so.*")
set(_renames "")

foreach(_lib ${_libs})
    get_filename_component(_name "${_lib}" NAME)
    get_filename_component(_dir  "${_lib}" DIRECTORY)

    if(IS_SYMLINK "${_lib}")
        file(REMOVE "${_lib}")
        continue()
    endif()

    string(REGEX REPLACE "(\\.so)\\..*$" "\\1" _base "${_name}")
    if(_base STREQUAL _name)
        continue()
    endif()

    execute_process(
        COMMAND patchelf --print-soname "${_lib}"
        OUTPUT_VARIABLE _old_soname
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _rc
    )
    if(NOT _rc EQUAL 0 OR _old_soname STREQUAL "")
        set(_old_soname "${_name}")
    endif()

    execute_process(
        COMMAND patchelf --set-soname "${_base}" "${_lib}"
        RESULT_VARIABLE _rc
    )
    if(NOT _rc EQUAL 0)
        message(FATAL_ERROR "patchelf --set-soname failed for ${_lib}")
    endif()

    file(RENAME "${_lib}" "${_dir}/${_base}")

    list(APPEND _renames "${_old_soname}|${_base}")
    if(NOT _old_soname STREQUAL _name)
        list(APPEND _renames "${_name}|${_base}")
    endif()
endforeach()

file(GLOB _dist_libs "${LIB_DIR}/*.so")
foreach(_bin ${_dist_libs})
    foreach(_pair ${_renames})
        string(REPLACE "|" ";" _p "${_pair}")
        list(GET _p 0 _old)
        list(GET _p 1 _new)
        execute_process(COMMAND patchelf --replace-needed "${_old}" "${_new}" "${_bin}"
                        ERROR_QUIET)
    endforeach()
endforeach()

file(GLOB _dist_bins "${BIN_DIR}/*")
foreach(_bin ${_dist_bins})
    foreach(_pair ${_renames})
        string(REPLACE "|" ";" _p "${_pair}")
        list(GET _p 0 _old)
        list(GET _p 1 _new)
        execute_process(COMMAND patchelf --replace-needed "${_old}" "${_new}" "${_bin}"
                        ERROR_QUIET)
    endforeach()
endforeach()
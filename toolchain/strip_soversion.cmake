file(GLOB _libs "${LIB_DIR}/*.so.*")

set(_renames "")

foreach(_lib ${_libs})
    get_filename_component(_name "${_lib}" NAME)

    string(REGEX REPLACE "(\\.so)\\..*$" "\\1" _base "${_name}")
    if(NOT _base STREQUAL _name)
        get_filename_component(_dir "${_lib}" DIRECTORY)
        set(_new "${_dir}/${_base}")

        execute_process(COMMAND patchelf --set-soname ${_base} "${_lib}")
        file(RENAME "${_lib}" "${_new}")

        list(APPEND _renames "${_name}:${_base}")
    endif()
endforeach()


file(GLOB _all_bins "${BIN_DIR}/*" "${LIB_DIR}/*.so")
foreach(_bin ${_all_bins})
    foreach(_pair ${_renames})
        string(REPLACE ":" ";" _pair_list "${_pair}")
        list(GET _pair_list 0 _old)
        list(GET _pair_list 1 _new)
        execute_process(
            COMMAND patchelf --replace-needed ${_old} ${_new} "${_bin}"
            ERROR_QUIET
        )
    endforeach()
endforeach()
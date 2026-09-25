find_package(PkgConfig REQUIRED)
pkg_check_modules(CRITERION REQUIRED criterion)

add_executable(test_wconr
    test_wizard.c
)
target_link_libraries(test_wconr
    wconr
    ${CRITERION_LIBRARIES}
)
target_include_directories(test_wconr PRIVATE ${CRITERION_INCLUDE_DIRS})

target_compile_options(test_wconr PRIVATE --coverage -O0 -g)
target_link_options(test_wconr PRIVATE --coverage)

add_test(NAME wconr_tests COMMAND test_wconr --verbose)

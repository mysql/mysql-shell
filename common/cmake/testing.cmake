

MACRO(SETUP_TESTING)
    include(CTest)
    include(gtest)

    # Target to update the test case file
    # Call 'make testgroups' to update the TestGroups.txt file after adding a new test case.
    add_custom_target(testgroups 
            ${CMAKE_BINARY_DIR}/unittest/run_unit_tests --generate_test_groups=${CMAKE_SOURCE_DIR}/unittest/TestGroups.txt
            )

    IF(WITH_COVERAGE)
        MESSAGE(STATUS "Enabling coverage support for gcc")

        ADD_DEFINITIONS("-g -O0 -Wall -W -Wshadow -Wunused-variable -Wunused-parameter -Wunused-function -Wunused -Wno-system-headers -Wno-deprecated -Woverloaded-virtual -Wwrite-strings -fprofile-arcs -ftest-coverage")
   
        SET(GCOV_LDFLAGS "-fprofile-arcs -ftest-coverage")
    ENDIF(WITH_COVERAGE)

ENDMACRO(SETUP_TESTING)

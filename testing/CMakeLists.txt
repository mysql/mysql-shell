

if(WITH_TESTS)
    add_definitions(-DGTEST_LANG_CXX11=0 ${GMOCK_CFLAGS})
    include_directories(${CMAKE_SOURCE_DIR}/testing/include 
                ${GTEST_INCLUDE_DIR} 
                ${CMAKE_SOURCE_DIR}/include
                ${MYSQL_INCLUDE_DIRS}
    )


    file(GLOB mysqlsh_tests_SRC
        "${PROJECT_SOURCE_DIR}/testing/*_t.cc"
        "${PROJECT_SOURCE_DIR}/testing/test_main.cc"
        "${PROJECT_SOURCE_DIR}/src/boost_code.cc"
    )
    add_executable(run_tests ${mysqlsh_tests_SRC})
    target_link_libraries(run_tests
            mysqlsh
            mysqlshmods
            gtest
            ${PROTOBUF_LIBRARY}
            ${GCOV_LDFLAGS}
            ${V8_LINK_LIST}
            ${MYSQL_LIBRARIES}
            )

    if (WIN32)
       #target_link_libraries( run_tests gtestd )
        target_link_libraries( run_tests libboost_filesystem-vc120-mt-sgd-1_56 )
    else()
        target_link_libraries( run_tests pthread )
        target_link_libraries( run_tests boost_filesystem )
    endif()

    include(TestGroups.txt)
else()
    message(WARNING "Skipping tests. To enable unit-tests use -DWITH_TESTS=1 -DWITH_GTEST=path")
endif()




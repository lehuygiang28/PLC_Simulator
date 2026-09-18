set(TEST_TARGET PLCSimulatorTests)

add_executable(${TEST_TARGET}
    main.cpp
    tst_DeviceAddress.h
    tst_DeviceAddress.cpp
    ${CMAKE_SOURCE_DIR}/src/Core/DeviceAddress.cpp
)

target_include_directories(${TEST_TARGET} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${TEST_TARGET} PRIVATE
    Qt${QT_VERSION}::Core
    Qt${QT_VERSION}::Test
)

target_compile_definitions(${TEST_TARGET} PRIVATE UNICODE _UNICODE)

set_target_properties(${TEST_TARGET} PROPERTIES
    WIN32_EXECUTABLE FALSE
)

add_test(NAME DeviceAddress COMMAND ${TEST_TARGET})

find_program(PLCSIM_WINDEPLOYQT windeployqt
    HINTS "$ENV{Qt6_DIR}/bin" "${Qt6_DIR}/../../../bin")
if(PLCSIM_WINDEPLOYQT)
    add_custom_command(TARGET ${TEST_TARGET} POST_BUILD
        COMMAND "${PLCSIM_WINDEPLOYQT}"
            $<$<CONFIG:Debug>:--debug>
            $<$<CONFIG:Release>:--release>
            --no-compiler-runtime
            --dir "$<TARGET_FILE_DIR:${TEST_TARGET}>"
            "$<TARGET_FILE:${TEST_TARGET}>"
        COMMENT "Deploying Qt runtime for PLCSimulatorTests")
endif()

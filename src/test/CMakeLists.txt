# COSMO 单元测试构建配置

# 辅助函数：添加测试可执行文件并注册到CTest
function(cosmo_add_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE libcosmo)
    add_test(NAME ${test_name}
        COMMAND ${test_name}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 30 LABELS "unit")
endfunction()

# 注册所有测试
cosmo_add_test(framework_test            framework_test.cpp)
cosmo_add_test(simulation_test           simulation_test.cpp)
cosmo_add_test(simulation_structure_test simulation_structure_test.cpp)
cosmo_add_test(simulation_listener_test  simulation_listener_test.cpp)
cosmo_add_test(scenario_test             scenario_test.cpp)
cosmo_add_test(csv_writer_test           csv_writer_test.cpp)
cosmo_add_test(simulation_runner_test    simulation_runner_test.cpp)
cosmo_add_test(cosmo_test                cosmo_test.cpp)
cosmo_add_test(cosmo_comprehensive_test  cosmo_test.cpp)
cosmo_add_test(cosmo_api_test            cosmo_api_test.cpp)
cosmo_add_test(data_test                 data_test.cpp)
cosmo_add_test(boundary_test             boundary_test.cpp)
cosmo_add_test(fmi_test                  fmi_test.cpp)
cosmo_add_test(ssp_test                  ssp_test.cpp)
cosmo_add_test(remote_test               remote_test.cpp)
cosmo_add_test(cli_test                  cli_test.cpp)
cosmo_add_test(filesystem_test           filesystem_test.cpp)
cosmo_add_test(fmi1_test                 fmi1_test.cpp)
cosmo_add_test(fmi3_test                 fmi3_test.cpp)
cosmo_add_test(fmu_processor_test        fmu_processor_test.cpp)
cosmo_add_test(data_router_test          data_router_test.cpp)
cosmo_add_test(mass_spring_damper_test   mass_spring_damper_test.cpp)
cosmo_add_test(fmu_multi_version_test   fmu_multi_version_test.cpp)
cosmo_add_test(fmu1_identity_test       fmu1_identity_test.cpp)

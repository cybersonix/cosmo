# COSMO 示例程序构建配置

# C++ 基本仿真示例
add_executable(basic_simulation basic_simulation.cpp)
target_link_libraries(basic_simulation PRIVATE libcosmo)

# 质量弹簧阻尼器联合仿真示例
add_executable(mass_spring_damper_sim mass_spring_damper_sim.cpp)
target_link_libraries(mass_spring_damper_sim PRIVATE libcosmo)

# C API 示例（用C++编译器链接以兼容C API头文件）
add_executable(c_api_example c_api_example.c)
target_include_directories(c_api_example PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(c_api_example PRIVATE libcosmo)

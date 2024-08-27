# add_executable(BiSearch main.cc)
# target_link_libraries(BiSearch ${FINAL_OBJ})

find_package(Threads REQUIRED)
# target_link_libraries(BiSearch ${CMAKE_THREAD_LIBS_INIT})

# 设置 Gurobi 库的路径
set(GUROBI_HOME /home/junwu/gurobi1102/linux64)
set(GUROBI_LIB ${GUROBI_HOME}/lib)
set(GUROBI_INCLUDE ${GUROBI_HOME}/include)
# 包含 Gurobi 头文件
include_directories(${GUROBI_INCLUDE})

add_executable(BiSearch main.cc)
target_link_libraries(BiSearch ${FINAL_OBJ} xdelta3 ${CMAKE_THREAD_LIBS_INIT} ${GUROBI_LIB}/libgurobi_c++.a ${GUROBI_LIB}/libgurobi110.so)

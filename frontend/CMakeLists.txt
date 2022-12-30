
#project(fronend)


 

#############################################
#               Build fronend.exe           #
#############################################

file(GLOB_RECURSE SRC_FRONTEND ${CMAKE_SOURCE_DIR}/frontend/*.cpp)
include_directories(${CMAKE_SOURCE_DIR}/frontend/)

add_executable(frontend.exe ${SRC_FRONTEND})


#############################################
#            Link our libraries             #
#############################################
#target_link_libraries(frontend.exe libPSI_Tests)
target_link_libraries(frontend.exe libOPRF)
target_link_libraries(frontend.exe libOTe)
set(Boost_NO_SYSTEM_PATHS ON)
find_package(Boost  COMPONENTS system thread)
message("frontend is using boost at: ${Boost_LIBRARIES}")
include_directories(${Boost_INCLUDE_DIR})
find_package(pxr CONFIG REQUIRED)

if (pxr_FOUND)
   find_package(PythonInterp 3 REQUIRED)
   find_package(PythonLibs 3 REQUIRED)
   list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/../Boost_1_76_0/bin")

   include_directories(
       ${PYTHON_INCLUDE_DIRS})

   target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
       ${PYTHON_LIBRARIES}
       usd)
endif()

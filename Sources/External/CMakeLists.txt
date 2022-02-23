add_subdirectory(websocketpp)

file(COPY websocketpp/websocketpp fmt/include/fmt json.hpp Logger.hpp WebSocketClient.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
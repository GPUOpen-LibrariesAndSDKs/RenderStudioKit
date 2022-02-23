add_subdirectory(websocketpp)

file(COPY websocketpp/websocketpp fmt/include/fmt json.hpp Logger.hpp WebSocketClient.h base64.hpp DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
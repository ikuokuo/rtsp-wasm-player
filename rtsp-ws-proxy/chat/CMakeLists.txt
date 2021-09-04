## chat-server

add_executable(chat-server
  main.cc
  chat_room.cc
  chat_server.cc
  chat_session.cc
  ../ws_server.cc
)
target_link_libraries(chat-server
  ${Boost_LIBRARIES} glog::glog yaml-cpp
)

install(TARGETS chat-server
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

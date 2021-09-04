## chat-client

add_executable(chat-client
  main.cc
)
target_link_libraries(chat-client
  ${Boost_LIBRARIES} glog::glog yaml-cpp
)

install(TARGETS chat-client
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

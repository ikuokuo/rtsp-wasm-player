#pragma once

#ifdef MY_USE_SSL

#include "ws_server_ssl.h"

using WsServer = WsServerSSL;

#else

#include "ws_server_plain.h"

#endif

// If wanna http & https works together, see example:
// Advanced, flex (plain + SSL)
//  https://github.com/boostorg/beast/blob/develop/example/advanced/server-flex/advanced_server_flex.cpp

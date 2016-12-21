//
// connection_manager.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER_CONNECTION_MANAGER_HPP
#define HTTP_SERVER_CONNECTION_MANAGER_HPP

#include <set>
#include <map>


/// Manages open connections so that they may be cleanly stopped when the server
/// needs to shut down.
template<class CONNECTION_HANDLER>
class ServerConnectionManager
{
public:
    ServerConnectionManager(const ServerConnectionManager&) = delete;
    ServerConnectionManager& operator=(const ServerConnectionManager&) = delete;

    /// Construct a connection manager.
    ServerConnectionManager();

    void registerConnectionHandler(CONNECTION_HANDLER* connHandler);
 
    /// deregister the specified connection.
    void deregister(CONNECTION_HANDLER* ch);
    /// Stop the specified connection.
    void stop(CONNECTION_HANDLER* ch);

    /// Stop all connections.
    void stop_all();

private:
  /// The managed connections.
#define CM_SMARTPOINTER  
#ifdef CM_SMARTPOINTER
  std::map<CONNECTION_HANDLER*, std::unique_ptr<CONNECTION_HANDLER>> _connections;
#else
  std::set<CONNECTION_HANDLER*> _connections;
#endif
};

#include "server_connection_manager.cpp"

#endif // HTTP_SERVER_CONNECTION_MANAGER_HPP

#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <string>
#include <signal.h>
#include <utility>

#include "marvin_error.hpp"
#include "server_connection_manager.hpp"
#include "request_handler_base.hpp"
#include "connection_interface.hpp"
#include "tcp_connection.hpp"
#include "tls_connection.hpp"
#include "message_reader_v2.hpp"
#include "message_writer_v2.hpp"
#include "rb_logger.hpp"
#include "connection_handler.hpp"

/**
* @brief HTTP server class.
* @discussion TRequestHandler is a template argument that must conform to RequestHandlerBase
* for a class that will handle an http request and send the necessary response.
*
* Discussion of structure
*
*   TRequestHandler is a "user" provided class instance that actually handles a one or more request
*   on a single connection.
*
*   connection_handler (ConnectionHandler) wraps a single instance of TRequestHandler and manages
*   the life cycle of that TRequestHandler instance, and thus represents the life time of a single client
*   connection.
*
*   server_connection_manager (ServerConnectionManager)
*   -   only one of these exists. That instance
*   -   keeps track of  a smart pointer to every active instane of ConnectionHandler
*       thereby ensures they stay in existence while active and can be deleted en-mass if/when the
*       server needs to close down
*   -   limits the number of active ConnectionHandler instances at any point in time to ensure that
*       the server does not get overloaded.
*
*/
template<class TRequestHandler> class HTTPServer
{
public:

    static void configSet_NumberOfThreads(int num);

    HTTPServer(const HTTPServer&) = delete;
    HTTPServer& operator=(const HTTPServer&) = delete;

    /**
    ** @brief Construct the server to listen on the specified TCP address and port.
    ** @param long port defaults to 9991
    */
    explicit HTTPServer();
    ~HTTPServer();
    /**
    ** @brief starts the listen process on the servers port, and from there
    ** dispatches instances of TRequestHandler to service the connection
    */
    void listen(long port = 9991);
    
private:

    static int __numberOfThreads;

    /**
    ** @brief just as it says - init the server ready to list
    */
    void initialize();
    
    /**
    ** @brief Initiates an asynchronous accept operation.
    */
    void startAccept();
    
    /**
    ** @brief callback that is invoked on completio of an accept call
    ** @param handler an object responsible for managing the new connection established by the
    **          completed accept call.
    ** @param err a boost errorcide that described any error condition
    */
    void handleAccept(ConnectionHandler<TRequestHandler>* handler, const boost::system::error_code& err);

    /**
    ** @brief encapsulates the process of posting a callback fn to the servcers strand
    */
    void postOnStrand(std::function<void()> fn);
    
    /**
    ** @brief sets up a signal callback
    */
    void waitForStop();
    
    /**
    ** @brief IS the signal callback
    */
    void doStop(const Marvin::ErrorType& err);
    
    int                                             _numberOfThreads;
    long                                            _port;
    boost::asio::io_service                         _io;
    boost::asio::strand                             _serverStrand;
    boost::asio::signal_set                         _signals;
    boost::asio::ip::tcp::acceptor                  _acceptor;
    ServerConnectionManager<ConnectionHandler<TRequestHandler>>   _connectionManager;

};
template <class TRequestHandler>
using Server_Template =  typename HTTPServer<TRequestHandler>::Server;

#include "http_server.ipp"
#endif // HTTP_SERVER_HPP

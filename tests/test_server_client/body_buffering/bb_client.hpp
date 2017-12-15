#ifndef test_message_client_hpp
#define test_message_client_hpp

#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <boost/asio.hpp>
#include <pthread.h>
#include "rb_logger.hpp"
#include "error.hpp"
#include "repeating_timer.hpp"
#include "bb_testcase.hpp"
#include "tcp_connection.hpp"
#include "message_reader.hpp"

namespace body_buffering {
    class TClient;
    typedef std:: shared_ptr<TClient> TClientSPtr;
    typedef std:: unique_ptr<TClient> TClientUPtr;

    typedef std::function<void(boost::system::error_code err)> SysErrorCb;
    typedef std::function<void(Marvin::ErrorType err)> MarvinErrorCb;
    /**
    * This is a client that sends a request as a raw set of buffers without
    * any checking on the method, headers, content length etc. Caller musrt ensure
    * all that stuff.
    * It reads the response as a http message
    */
    class TClient
    {
        public:
            TClient(boost::asio::io_service& io, std::string port, Testcase tc);

            void exec();
            /**
            * send all test case buffers
            */
            void send_testcase_buffers(SysErrorCb cb);
            /**
            * Establish connection
            */
            void connect();
            /**
            * Start the writing on one buffer
            */
            void write_line();
            /**
            * Handle completion of writing one buffer, either start the next buffer or end
            */
            void handle_write_complete(Marvin::ErrorType& err, std::size_t bytes_transfered);
        
            void read_message();
            void read_msg_complete_handler();
            void onMessage(Marvin::ErrorType er);
        
            #if 0
            void onBody(Marvin::ErrorType er, BufferChain chunk);
            void onHeaders(Marvin::ErrorType er);
            #endif
            /**
            * Puts a delay before each buffer write
            */
            void wait_before_write();
        
            boost::asio::io_service&    _io;
            std::string                 _scheme;
            std::string                 _server;
            std::string                 _port;
            Testcase                    _testcase;
            TCPConnectionSPtr           _conn_sptr;
            MessageReaderSPtr         _rdr;
            int                         _buffer_index; /// index into the vector of buffers in the test case
            MarvinErrorCb               _test_cb;
            boost::asio::deadline_timer _timer;

    };
}
#endif

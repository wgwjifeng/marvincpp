//
// The main entry point for Marvin - a mitm proxy for http/https 
//


#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <boost/asio.hpp>
#include <pthread.h>
#include "rb_logger.hpp"
RBLOGGER_SETLEVEL(LOG_LEVEL_INFO)

#include "server.hpp"
#include "request_handler_base.hpp"
#include "request.hpp"
#include "forwarding_handler.hpp"

class CollectorBase
{
    public:
        static bool             _firstTime;
        static NullCollector*   _instance;
    
        static NullCollector* getInstance(boost::asio::io_service& io);
        /**
        ** Delete copy constructors
        **/
        NullCollector(NullCollector const&)   = delete;
        void operator=(NullCollector const&)  = delete;
    
        /**
        ** This method actually implements the collect function but run on a dedicated
        ** strand. Even if this method does IO-wait operations the other thread will
        ** keep going
        **/
        void postedCollect(MessageReaderSPtr req, MessageWriterSPtr resp);

        /**
        ** Interface method for client code to call collect
        **/
        void collect(MessageReaderSPtr req, MessageWriterSPtr resp);
    
    private:
        NullCollector(boost::asio::io_service& io): _ioLoop(io), _myStrand(io);

        boost::asio::strand         _myStrand;
        boost::asio::io_service&    _ioLoop;
        std::ofstream               _outPipe;
        bool                        _pipeOpen;
};
bool NullCollector::_firstTime = true;
NullCollector* NullCollector::_instance = NULL;

void RunMitmServer()
{
    Server<ForwardingHandler<NullCollector>> server;
    server.listen(9991);
}

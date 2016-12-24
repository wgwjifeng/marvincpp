//
//  ConnectionHandler.cpp
//  MarvinCpp
//
//  Created by ROBERT BLACKWELL on 12/12/16.
//  Copyright © 2016 Blackwellapps. All rights reserved.
//

template<class TRequestHandler>
ConnectionHandler<TRequestHandler>::ConnectionHandler(
    boost::asio::io_service&                                        io,
    ServerConnectionManager<ConnectionHandler<TRequestHandler>>&    connectionManager,
    ConnectionInterface*                                            conn):  _io(io), _connectionManager(connectionManager)
{
    _requestHandlerPtr  = new TRequestHandler();
    _requestHandlerUnPtr = std::unique_ptr<TRequestHandler>(_requestHandlerPtr);
    
#ifdef CON_SMARTPOINTER
    _connection = std::shared_ptr<ConnectionInterface>(conn);
#else
    _connection = conn;
#endif
}


template<class TRequestHandler>
ConnectionHandler<TRequestHandler>::~ConnectionHandler()
{
#ifdef CON_SMARTPOINTER
#else
    delete _connection;
#endif
    LogDebug("");
    
}
template<class TRequestHandler>
void ConnectionHandler<TRequestHandler>::close()
{
    LogDebug(" fd:", nativeSocketFD());
//    _connection->close();
}

template<class TRequestHandler>
int ConnectionHandler<TRequestHandler>::nativeSocketFD()
{
    return _connection->nativeSocketFD();
}

template<class TRequestHandler>
void ConnectionHandler<TRequestHandler>::handleConnectComplete(bool hijacked)
{
    // do not want the connction closed unless !hijacked
    LogDebug(" fd:", nativeSocketFD());
    
    if( ! hijacked )
        _connection->close();
    
    _connectionManager.deregister(this); // should be maybe called deregister
}

template<class TRequestHandler>
void ConnectionHandler<TRequestHandler>::requestComplete()
{
    // a stub for future expansionto have a handler handle
    // multiple requests from the same connection
}

template<class TRequestHandler>
void ConnectionHandler<TRequestHandler>::handlerComplete()
{
    LogDebug(" fd:", nativeSocketFD());
    _connection->close();
    //
    // This call will start the process of deleting linked objects. Hence we need to have closed the
    // connection before this because after it we may not have the connection to close
    //
    _connectionManager.stop(this); // should be maybe called deregister
    
}

template<class TRequestHandler>
void ConnectionHandler<TRequestHandler>::readMessageHandler(Marvin::ErrorType& err)
{
    LogDebug(" fd:", nativeSocketFD());
    if( err ){
        LogDebug("error value: ", err.value(),
            " category: ", err.category().name(),
            " msg: ", err.category().message(err.value()));
            //
            // On read error do not call the handler - simply abort the request
            //
            this->handlerComplete();
    } else{
        if(_reader->method() == HttpMethod::CONNECT ){
//             _requestHandler.handleConnect(_io, *_reader, _connection, [this](bool hijack){
             _requestHandlerUnPtr->handleConnect(_io, *_reader, _connection, [this](bool hijack){
                this->handleConnectComplete(hijack);
             });
            
            //
            // by the time this returns anything needed has been saved or retained
            // so we can exit the connection handler
            //
        } else {
            _requestHandlerUnPtr->handleRequest(_io, *_reader, *_writer, [this](bool good){
//            _requestHandler.handleRequest(_io, *_reader, *_writer, [this](bool good){
                LogDebug("");
                //
                // @TODO should check here for Connection::close/keep-alive
                // and if keep-alive call requestComplete to read another request
                //
                this->handlerComplete();
            } );
        }
    }
}
//
// Gets the connection handler going
//
template<class TRequestHandler>
void ConnectionHandler<TRequestHandler>::serve()
{
    LogDebug(" fd:", nativeSocketFD());
    // set up reader and writer
#ifdef CON_SMARTPOINTER
    ConnectionInterface* cptr = _connection.get();
#else
    ConnectionInterface* cptr = _connection;
#endif
    
#ifdef CH_SMARTPOINTER
    _reader = std::unique_ptr<MessageReader>(new MessageReader(cptr, _io));
    _writer = std::unique_ptr<MessageWriter>(new MessageWriter(_io, false));
#else
    _reader = new MessageReader(_connection, _io);
    _writer = new MessageWriter(_io, false);
#endif
    _writer->setWriteSock(cptr);
    auto rmh = std::bind(&ConnectionHandler::readMessageHandler, this, std::placeholders::_1 );
    _reader->readMessage(rmh);
    
}

#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <boost/asio.hpp>
#include <pthread.h>
#include "rb_logger.hpp"
RBLOGGER_SETLEVEL(LOG_LEVEL_DEBUG)
#include "error.hpp"
#include "repeating_timer.hpp"
#include "tcp_connection.hpp"
#include "message_reader_v2.hpp"

#include "bb_testcase.hpp"
#include "bb_client.hpp"

using namespace body_buffering;

TClient::TClient(boost::asio::io_service& io, std::string port, Testcase tc)
: _io(io), _scheme("http"), _server("localhost"), _port(port), _testcase(tc), _timer(_io)
{
    _conn_sptr = std::shared_ptr<TCPConnection>(new TCPConnection(_io, _scheme, _server, _port));
}
void TClient::exec()
{
    LogDebug("");
    _buffer_index = 0;
    _test_cb = nullptr;
    connect();
}
void TClient::send_testcase_buffers(SysErrorCb cb)
{
    LogDebug("");
    _buffer_index = 0;
    _test_cb = cb;
    connect();
}

void TClient::connect()
{
    LogDebug("");
    _conn_sptr->asyncConnect([this](Marvin::ErrorType& err, ConnectionInterface* conn) {
        LogDebug("connected");
        if( ! err ){
            _rdr = std::make_shared<MessageReaderV2>(_io, _conn_sptr);
            auto wbf = std::bind(&TClient::wait_before_write, this);
            _io.post(wbf);
        } else {
            Marvin::ErrorType me = err;
            LogError("error_value", err.value(), " message: ", err.message());
            _test_cb(me);
        }
    });

}

void TClient::write_line()
{
    LogDebug("");
    std::string line = _testcase.lineAt(_buffer_index);
    LogDebug(" line: ", line);
    if (line == "eof" ) {
        _conn_sptr->shutdown();
    } else if (line == "close") {
        _conn_sptr->close();
    } else {
        auto hf = std::bind(&TClient::handle_write_complete, this, std::placeholders::_1, std::placeholders::_2);
        auto buf = boost::asio::buffer(line.c_str(), line.size());
        
        _conn_sptr->asyncWrite(line, hf);
    }
}
void TClient::handle_write_complete(Marvin::ErrorType& err, std::size_t bytes_transfered)
{
    LogDebug("");
    if( !err) {
        _buffer_index++;
        if(_buffer_index >= _testcase.buffers().size()) {
            Marvin::ErrorType err_val = err;
//            _test_cb(err_val);  /// finished writing the request message - let the reader collect the reponse
        } else {
            write_line();
        }
    } else {
        Marvin::ErrorType err_val = err;
        _test_cb(err_val);
    }
}
void TClient::wait_before_write()
{
    LogDebug("");
    _timer.expires_from_now(boost::posix_time::milliseconds(100));
//    auto ds = boost::bind(&TClient::write_line, this, std::placeholders::_1);
    _timer.async_wait([this](const boost::system::error_code& err) {
        read_message();
        write_line();
    });

}

/**
* runs a test that reads reads a full message
*/
void TClient::read_message()
{
    LogDebug("getting started");
//        makeReader();
    auto h = std::bind(&TClient::onMessage, this, std::placeholders::_1);
    _rdr->readMessage(h);
}

void TClient::onMessage(Marvin::ErrorType er)
{
    LogDebug("");
    Marvin::ErrorType expected_err = _testcase.result_onheaders_err();
    std::string exp_s = expected_err.message();
    std::string ers = er.message();
    if( er != expected_err){
        std::cout << "bad" << std::endl;
    }
//        assert(er == expected_err);
    assert(_rdr->statusCode() == _testcase.result_status_code());
    REQUIRE(_rdr->statusCode() == _testcase.result_status_code());
    auto h1 = _testcase.result_headers();
    auto h2 = _rdr->getHeaders();
    bool hh = (h1 == h2);
//    assert(_testcase.result_headers() == _rdr->getHeaders());
//    REQUIRE(_testcase.result_headers() == _rdr->getHeaders());
    auto b1 = _testcase.result_body();
    auto b2 = _rdr->get_body_chain();
    auto b3 = _rdr->get_raw_body_chain();
    auto s2 = b2.to_string();
    auto s3 = b3.to_string();
    assert(b1 == s2);
    REQUIRE(b1 == s2);
    auto desc = _testcase.getDescription();
    std::cout << "TestRunner::readMessage Success for testcase " << _testcase.getDescription() <<std::endl;
}

#if 0
/**
* runs a test that reads the headers only
*/
void TClient::run_StreamingBodyRead()
{
    LogDebug("getting started");
//        makeReader();
    auto h = std::bind(&Testrunner::onHeaders, this, std::placeholders::_1);
    _rdr->readHeaders(h);

}
#endif



#if 0
void TClient::onBody(Marvin::ErrorType er, BufferChain chunk)
{
    LogDebug(" entry");
    // are we done - if not hang another read
    auto bh = std::bind(&Testrunner::onBody, this, std::placeholders::_1, std::placeholders::_2);
    bool done = (er == Marvin::make_error_eom());
    body_accumulator += chain_to_string(chunk);
    if( done )
    {
        std::string expectedBody = _testcase.result_body();
        bool vb = _testcase.verify_body(body_accumulator);
        assert(vb);
        assert(er == Marvin::make_error_eom());
        REQUIRE(vb);
        REQUIRE(er == Marvin::make_error_eom());
        auto desc = _testcase.getDescription();
        std::cout << "TestRunner::run_StreamingBodyRead Success testcase " << _testcase.getDescription() <<std::endl;

    }else{
        _rdr->readBody(bh);
    }
    LogDebug("exit");
    
}
void Testrunner::onHeaders(Marvin::ErrorType er){
    LogDebug("entry");
    Marvin::ErrorType expected_err = _testcase.result_onheaders_err();
    std::string ers = er.message();
    assert(er == expected_err);
    assert(_rdr->statusCode() == _testcase.result_status_code());
    REQUIRE(er == expected_err);
    REQUIRE(_rdr->statusCode() == _testcase.result_status_code());
    auto h1 = _testcase.result_headers();
    auto h2 = _rdr->getHeaders();
    bool hhh = _testcase.verify_headers(h2);
    assert(hhh);
    REQUIRE(hhh);
    REQUIRE(h1 == h2);
    auto bh = std::bind(&Testrunner::onBody, this, std::placeholders::_1, std::placeholders::_2);
//        std::cout << "TestRunner::run_StreamingBodyRead Success testcase " << tcObj.getDescription() <<std::endl;
    _rdr->readBody(bh);
    LogDebug("exit");
}

#endif

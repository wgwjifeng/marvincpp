//
// Start both a server and a number of client tthreads and have each of the client
// threads make a number of requests from the server.
//
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <boost/asio.hpp>
#include <thread>
#include <pthread.h>
#include <gtest/gtest.h>

#include "rb_logger.hpp"
RBLOGGER_SETLEVEL(LOG_LEVEL_INFO)

#include "http_server.hpp"
#include "request_handler_base.hpp"
#include "bf_post.hpp"
#include "bf_pipeline.hpp"
#include "bf_testcase.hpp"
#include "bf_req_handler.hpp"
using namespace body_format;

static Testcase tcase01(
        TestType::WITH_STRING,
        "With string - 2 buffers",
        {
            "12345987654",
            "93hdiuybk"
        }
    );
static Testcase  tcase02(
        TestType::WITH_MBUFFER,
        "With mbuffer - 2 buffers",
        {
            "1234567890",
            "oiuhgre76",
            "HGYTRESAWQ"
        }
    );
static Testcase tcase03(
        TestType::WITH_BUFFER_CHAIN,
        "With buffer chain - 2 buffers",
        {
            "1234567890",
            "1m2j3k4i5u6",
            "qkjgtaitsko",
            "2"
        }
    );


namespace {

std::vector<body_format::Testcase> make_cases()
{
    std::vector<body_format::Testcase> cases = {
        /**
        * Testcases
        */
        body_format::Testcase(
                body_format::TestType::WITH_STRING,
                "With string - 2 buffers 00",
                {
                    "12345987654",
                    "93hdiuybk"
                }
        ),
        body_format::Testcase(
                body_format::TestType::WITH_MBUFFER,
                "With mbuffer - 2 buffers 01",
                {
                    "1234567890",
                    "oiuhgre76",
                    "HGYTRESAWQ"
                }
        ),
        body_format::Testcase(
                body_format::TestType::WITH_BUFFER_CHAIN,
                "With buffer chain - 2 buffers 03",
                {
                    "1234567890",
                    "1m2j3k4i5u6",
                    "qkjgtaitsko",
                    "2"
                }
        )
    };
    return cases;
}
    /**
    * Class that knows how to start and stop a server on port 9991
    * used in Testcase SetUp and TearDown
    */
    class ServerRunner
    {
    public:
        HTTPServer* server_ptr;
        std::thread* server_thread_ptr;
        ServerRunner(){};
        void setup()
        {
            static std::thread* server_thread_ptr = new std::thread([this](){
                try {
                    HTTPServer* server_ptr = new HTTPServer([](boost::asio::io_service& io){
                        return new body_format::RequestHandler(io);
                    });
                    this->server_ptr = server_ptr;
                    server_ptr->listen();
                    std::cout << __FUNCTION__ << " after listen" << std::endl;
                } catch(std::exception & ex) {
                    return;
                }
            });
            this->server_thread_ptr = server_thread_ptr;
        }
        void teardown()
        {
            sleep(2);
            this->server_ptr->terminate();
            this->server_thread_ptr->join();
        }
    };
    /**
    * Fixture that does nothing more than start the server
    * Use as base for other fixtures
    */
    class RequireServer : public ::testing::Test
    {
        public:
        virtual void SetUp(){}
        static void SetUpTestCase();
        virtual void Teardown(){}
        static void TearDownTestCase();
        static ServerRunner    _runner;
    };
    ServerRunner RequireServer::_runner;
    void RequireServer::SetUpTestCase()
    {
//        _runner.setup();
        std::cout << __FUNCTION__ << " " << std::endl;
    }
    void RequireServer::TearDownTestCase()
    {
        std::cout << __FUNCTION__ << " " << std::endl;
//        _runner.teardown();
    }


    /**
    * Parameterized fixture for testing each message stand alone
    * which requires a server to be started
    */
    class RoundTripOneshot : public RequireServer, public ::testing::WithParamInterface<body_format::Testcase>
    {
        public:
        RoundTripOneshot(): _tc(GetParam())
        {}
        ~RoundTripOneshot(){}
        virtual void SetUp(){}
        virtual void TearDown() {}
        body_format::Testcase   _tc;
    };

    /**
    * Fixture that does nothing more than start the server
    */
    class Multiple : public RequireServer
    {
    };

}
// run each test on a separate io_service
TEST_P(RoundTripOneshot, 001)
{
    const body_format::Testcase& testcase = GetParam();
    boost::asio::io_service io_service;
    body_format::PostTest tst(io_service, testcase);
    tst.exec();
    std::cout << testcase._description << std::endl;
    io_service.run();
}

INSTANTIATE_TEST_CASE_P(consecutive, RoundTripOneshot, testing::Values(tcase01, tcase02, tcase03));
#if 1
// run all request on a single io_service at the same time
// but all as separate request streams and separate connections
TEST_F(Multiple, alltogether)
{
    boost::asio::io_service io_service;
    std::vector<body_format::Testcase> cases = make_cases();
    std::vector<std::shared_ptr<body_format::PostTest>> saved;
    for(const body_format::Testcase& c : cases) {
        std::shared_ptr<body_format::PostTest> texec = std::shared_ptr<body_format::PostTest>(new body_format::PostTest(io_service, c));
        saved.push_back(texec);
        texec->exec();
    }
    io_service.run();
}
#endif
#if 1
TEST_F(Multiple, pipeline)
{
    boost::asio::io_service io_service;
    std::vector<body_format::Testcase> cases = make_cases();
    body_format::PipelineTest tst(io_service, cases);
    tst.exec();
    io_service.run();
}
#endif
int main( int argc, char* argv[] )
{
    // global setup - run a server
    RBLogging::setEnabled(false);
#define EX_RUNNER
#ifdef EX_RUNNER
    ServerRunner s_runner;
    s_runner.setup();
#endif
    char* _argv[2] = {argv[0], (char*)"--gtest_filter=*.*"}; // change the filter to restrict the tests that are executed
    int _argc = 2;
    testing::InitGoogleTest(&_argc, _argv);
    auto res = RUN_ALL_TESTS();
#ifdef EX_RUNNER
    sleep(2);
    s_runner.teardown();
#endif
    return res;
}
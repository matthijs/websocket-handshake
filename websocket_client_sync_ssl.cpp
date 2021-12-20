//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket SSL client, synchronous
//
//------------------------------------------------------------------------------

#include "root_certificates.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <cstdlib>
#include <future>
#include <iostream>
#include <string>

namespace beast = boost::beast;        // from <boost/beast.hpp>
namespace http = beast::http;          // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;// from <boost/beast/websocket.hpp>
namespace net = boost::asio;           // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;      // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;      // from <boost/asio/ip/tcp.hpp>

namespace console {
    std::mutex iomutex;
    template<class... Args>
    void
    println(Args const &...args)
    {
        using expand = int[];

        std::lock_guard<std::mutex> g{iomutex};
        expand{0, ((std::cout << args), 0)...};
        std::cout << '\n';
    }
}// namespace console

// Sends a WebSocket message and prints the response

void
sync_test(net::io_context &ioc, ssl::context &sslctx, std::string host,
          std::string port, std::string path, std::string text)
try
{

    // These objects perform our I/O
    tcp::resolver resolver{ioc};
    websocket::stream<beast::ssl_stream<tcp::socket>> ws{ioc, sslctx};

    // Look up the domain name
    auto const results = resolver.resolve(host, port);

    // Make the connection on the IP address we get from a lookup
    auto ep = net::connect(get_lowest_layer(ws), results);

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str()))
        throw beast::system_error(
        beast::error_code(static_cast<int>(::ERR_get_error()),
                          net::error::get_ssl_category()),
        "Failed to set SNI Hostname");

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    host += ':' + std::to_string(ep.port());

    // Perform the SSL handshake
    ws.next_layer().handshake(ssl::stream_base::client);

    // Set a decorator to change the User-Agent of the handshake
    ws.set_option(
    websocket::stream_base::decorator([](websocket::request_type &req) {
        req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                " websocket-client-coro");
    }));

    // Perform the websocket handshake
    boost::beast::websocket::response_type response;
    boost::system::error_code ec;
    ws.handshake(response, host, path, ec);
    console::println("[sync] ", ec.message());
    console::println("[sync] ", response);

    if (ec)
        return;

    // Send the message
    ws.write(net::buffer(std::string(text)));

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    // Read a message into our buffer
    ws.read(buffer);

    // Close the WebSocket connection
    ws.close(websocket::close_code::normal);

    // If we get here then the connection is closed gracefully

    // The make_printable() function helps print a ConstBufferSequence
    console::println("[sync] ", beast::make_printable(buffer.data()));

} catch (std::exception &e)
{
    console::println("[sync] ", "Error: ", e.what());
}

boost::asio::awaitable<void>
async_test(ssl::context &sslctx, std::string host,
           std::string port, std::string path, std::string text)
try
{
    using boost::asio::redirect_error;
    using boost::asio::use_awaitable;

    auto exec = co_await boost::asio::this_coro::executor;

    // These objects perform our I/O
    tcp::resolver resolver{exec};
    websocket::stream<beast::ssl_stream<tcp::socket>> ws{exec, sslctx};

    // Look up the domain name
    auto const results = co_await resolver.async_resolve(host, port, use_awaitable);

    // Make the connection on the IP address we get from a lookup
    auto ep = co_await net::async_connect(get_lowest_layer(ws), results, use_awaitable);

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str()))
        throw beast::system_error(
        beast::error_code(static_cast<int>(::ERR_get_error()),
                          net::error::get_ssl_category()),
        "Failed to set SNI Hostname");

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    host += ':' + std::to_string(ep.port());

    // Perform the SSL handshake
    co_await ws.next_layer().async_handshake(ssl::stream_base::client, use_awaitable);

    // Set a decorator to change the User-Agent of the handshake
    ws.set_option(
    websocket::stream_base::decorator([](websocket::request_type &req) {
        req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                " websocket-client-coro");
    }));

    // Perform the websocket handshake
    boost::beast::websocket::response_type response;
    boost::system::error_code ec;
    co_await ws.async_handshake(response, host, path, redirect_error(use_awaitable, ec));
    console::println("[async] ", ec.message());
    console::println("[async] ", response);

    if (ec)
        co_return;

    // Send the message
    co_await ws.async_write(net::buffer(std::string(text)), use_awaitable);

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    // Read a message into our buffer
    co_await ws.async_read(buffer, use_awaitable);

    // Close the WebSocket connection
    co_await ws.async_close(websocket::close_code::normal, use_awaitable);

    // If we get here then the connection is closed gracefully

    // The make_printable() function helps print a ConstBufferSequence
    console::println("[async] ", beast::make_printable(buffer.data()));

} catch (std::exception &e)
{
    console::println("[async] ", "Error: ", e.what());
}

int
main(int argc, char **argv)
{
    // Check command line arguments.
    if (argc != 4)
    {
        std::cerr << "Usage: websocket-client-sync-ssl <host> <port> <text>\n"
                  << "Example:\n"
                  << "    websocket-client-sync-ssl echo.websocket.org 443 "
                     "\"Hello, world!\"\n";
        return EXIT_FAILURE;
    }
    std::string host = argv[1];
    auto const port = argv[2];
    auto const text = argv[3];

    // The io_context is required for all I/O
    net::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::tlsv12_client};

    // This holds the root certificate used for verification
    load_root_certificates(ctx);

    auto sync_future = std::async(std::launch::async, [=, &ioc, &ctx] {
        sync_test(ioc, ctx, host, port, "/401", text);
    });

    boost::asio::co_spawn(ioc, async_test(ctx, host, port, "/401", text), boost::asio::detached);

    ioc.run();
    sync_future.wait();


    return EXIT_SUCCESS;
}

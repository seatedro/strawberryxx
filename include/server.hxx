#ifndef SERVER_HXX
#define SERVER_HXX

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include "database.hxx"

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

// We'll handle each connection in a Session object
class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
public:
    HttpConnection(net::ip::tcp::socket socket, std::shared_ptr<Database> db);
    void start();

private:
    void doRead();
    void onRead(beast::error_code ec, std::size_t bytes_transferred);
    void handleRequest(http::request<http::string_body>&& req);
    void sendResponse(http::response<http::string_body>&& res);

    http::response<http::string_body> handleLogin(const http::request<http::string_body>& req);
    http::response<http::string_body> handleProfile(const http::request<http::string_body>& req);
    http::response<http::string_body> handleLogout(const http::request<http::string_body>& req);

    net::ip::tcp::socket            socket_;
    beast::flat_buffer              buffer_;
    std::shared_ptr<Database>       db_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
};

class Server
{
public:
    Server(net::io_context& ioc, unsigned short port, std::shared_ptr<Database> db);

    void start();

private:
    void doAccept();

    net::ip::tcp::acceptor  acceptor_;
    net::io_context&        ioc_;
    std::shared_ptr<Database> db_;
};

#endif // SERVER_HXX

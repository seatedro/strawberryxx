#include "server.hxx"
#include "boost/beast/http/impl/read.hpp"
#include "database.hxx"
#include "logger.hxx"
#include "session_manager.hxx"

//
// HttpConnection
//

HttpConnection::HttpConnection(net::ip::tcp::socket socket,
                               std::shared_ptr<Database> db)
    : socket_(std::move(socket)), db_(db) {}

void HttpConnection::start() { doRead(); }

void HttpConnection::doRead() {
  auto self = shared_from_this();
  http::async_read(
      socket_, buffer_, req_,
      [this, self](beast::error_code ec, std::size_t bytes_transferred) {
        onRead(ec, bytes_transferred);
      });
}

void HttpConnection::onRead(beast::error_code ec,
                            std::size_t bytes_transferred) {
  if (ec == http::error::end_of_stream) {
    // Graceful close
    socket_.shutdown(net::ip::tcp::socket::shutdown_send, ec);
    return;
  }
  if (ec) {
    Logger::getLogger()->error("Read error: {}", ec.message());
    return;
  }

  // http::request<http::string_body> req;
  // // buffer_ already has the data
  // // parse it into the HTTP request struct
  // auto bytes_used = http::parser<false, http::string_body>{};
  // bytes_used.put(buffer_.data(), ec);
  // if (ec) {
  //   Logger::getLogger()->error("Parser error: {}", ec.message());
  //   return;
  // }
  // if (bytes_used.is_done()) {
  //   req = bytes_used.get();
  //   buffer_.consume(bytes_used.release());
  // }

  handleRequest(std::move(req_));
}

void HttpConnection::handleRequest(http::request<http::string_body> &&req) {
  http::response<http::string_body> res;

  // Route matching
  auto target = std::string(req.target());
  if (req.method() == http::verb::post && target == "/login") {
    res = handleLogin(req);
  } else if (req.method() == http::verb::get && target == "/profile") {
    res = handleProfile(req);
  } else if (req.method() == http::verb::post && target == "/logout") {
    res = handleLogout(req);
  } else {
    res.result(http::status::not_found);
    res.body() = "Route not found";
    res.prepare_payload();
  }

  sendResponse(std::move(res));
}

void HttpConnection::sendResponse(http::response<http::string_body> &&res) {
  auto self = shared_from_this();

  http::async_write(
      socket_, res, [this, self](beast::error_code ec, std::size_t) {
        // If an error occurs on the connection, or if the request was HTTP/1.0
        // and the "Connection: close" was specified, we might close the socket.
        if (ec) {
          Logger::getLogger()->error("Write error: {}", ec.message());
        }

        // For simplicity, close the socket after every response in this example
        beast::error_code ignored_ec;
        socket_.shutdown(net::ip::tcp::socket::shutdown_send, ignored_ec);
      });
}

// ========== ROUTE IMPLEMENTATIONS ==========

// POST /login
// Expect JSON body: { "username": "...", "password": "..." }
http::response<http::string_body>
HttpConnection::handleLogin(const http::request<http::string_body> &req) {
  http::response<http::string_body> res;

  auto body = req.body();
  std::string username, password;

  auto userPos = body.find("\"username\":");
  auto passPos = body.find("\"password\":");
  if (userPos != std::string::npos && passPos != std::string::npos) {
    // This is extremely naive parsing
    // We'll just assume the format is: "username":"alice","password":"secret"
    auto userStart = body.find("\"", userPos + 11) + 1;
    auto userEnd = body.find("\"", userStart);
    username = body.substr(userStart, userEnd - userStart);

    auto passStart = body.find("\"", passPos + 10) + 1;
    auto passEnd = body.find("\"", passStart);
    password = body.substr(passStart, passEnd - passStart);
  }

  // Query the DB to verify user
  bool isValid = false;
  {
    auto logger = Logger::getLogger();
    auto allUsers = db_->getAllUsers();
    for (auto &u : allUsers) {
      if (u.username == username && u.password == password) {
        isValid = true;

        // Create session
        std::string sessionToken =
            SessionManager::instance().createSession(u.id);

        // Return a Set-Cookie header
        res.result(http::status::ok);
        res.set(http::field::set_cookie,
                "sessionId=" + sessionToken + "; HttpOnly");
        res.body() = "Login successful. Session created.\n";
        res.prepare_payload();
        logger->info("User '{}' logged in. Session token: {}", username,
                     sessionToken);
        return res;
      }
    }
  }

  // If we get here, invalid
  res.result(http::status::unauthorized);
  res.body() = "Invalid credentials\n";
  res.prepare_payload();
  return res;
}

// GET /profile
// Expect a 'sessionId' cookie to retrieve user info
http::response<http::string_body>
HttpConnection::handleProfile(const http::request<http::string_body> &req) {
  http::response<http::string_body> res;

  // Extract 'sessionId' from Cookie
  auto it = req.find(http::field::cookie);
  if (it == req.end()) {
    res.result(http::status::unauthorized);
    res.body() = "No session cookie found\n";
    res.prepare_payload();
    return res;
  }

  // Very naive cookie parse
  auto cookieValue = std::string(it->value());
  auto pos = cookieValue.find("sessionId=");
  if (pos == std::string::npos) {
    res.result(http::status::unauthorized);
    res.body() = "Session cookie not found\n";
    res.prepare_payload();
    return res;
  }
  pos += 10; // skip "sessionId="
  auto endPos = cookieValue.find(";", pos);
  auto sessionToken = (endPos == std::string::npos)
                          ? cookieValue.substr(pos)
                          : cookieValue.substr(pos, endPos - pos);

  auto userIdOpt = SessionManager::instance().validateSession(sessionToken);
  if (!userIdOpt) {
    res.result(http::status::unauthorized);
    res.body() = "Invalid or expired session\n";
    res.prepare_payload();
    return res;
  }

  int userId = *userIdOpt;
  auto user = db_->getUserById(userId);
  if (user.id == 0) {
    // user not found
    res.result(http::status::not_found);
    res.body() = "User not found\n";
    res.prepare_payload();
    return res;
  }

  res.result(http::status::ok);
  res.set(http::field::content_type, "application/json");
  res.body() = "{ \"id\": " + std::to_string(user.id) + ", \"username\": \"" +
               user.username + "\" }";
  res.prepare_payload();
  return res;
}

// POST /logout
// Invalidate the session
http::response<http::string_body>
HttpConnection::handleLogout(const http::request<http::string_body> &req) {
  http::response<http::string_body> res;

  auto it = req.find(http::field::cookie);
  if (it == req.end()) {
    res.result(http::status::unauthorized);
    res.body() = "No session cookie found\n";
    res.prepare_payload();
    return res;
  }

  auto cookieValue = std::string(it->value());
  auto pos = cookieValue.find("sessionId=");
  if (pos == std::string::npos) {
    res.result(http::status::unauthorized);
    res.body() = "Session cookie not found\n";
    res.prepare_payload();
    return res;
  }
  pos += 10;
  auto endPos = cookieValue.find(";", pos);
  auto sessionToken = (endPos == std::string::npos)
                          ? cookieValue.substr(pos)
                          : cookieValue.substr(pos, endPos - pos);

  // Invalidate
  SessionManager::instance().invalidateSession(sessionToken);

  res.result(http::status::ok);
  res.body() = "Logged out\n";
  res.prepare_payload();
  return res;
}

//
// Server
//

Server::Server(net::io_context &ioc, unsigned short port,
                       std::shared_ptr<Database> db)
    : acceptor_(ioc, {net::ip::make_address("0.0.0.0"), port}), ioc_(ioc),
      db_(db) {}

void Server::start() {
  Logger::getLogger()->info("HTTP server listening on port {}",
                            acceptor_.local_endpoint().port());
  doAccept();
}

void Server::doAccept() {
  acceptor_.async_accept(
      [this](beast::error_code ec, net::ip::tcp::socket socket) {
        if (!ec) {
          // Launch the session for handling the connection
          std::make_shared<HttpConnection>(std::move(socket), db_)->start();
        } else {
          Logger::getLogger()->error("Accept error: {}", ec.message());
        }

        // Accept next
        doAccept();
      });
}

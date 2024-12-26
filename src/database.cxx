#include "database.hxx"
#include "logger.hxx"
#include <libpq-fe.h>
#include <sstream>
#include <stdexcept>

Database::Database(const std::string &conninfo)
    : m_connInfo(conninfo), m_connection(nullptr) {}

Database::~Database() {
  if (m_connection) {
    PQfinish(m_connection);
    m_connection = nullptr;
  }
}

bool Database::connect() {
  auto logger = Logger::getLogger();
  m_connection = PQconnectdb(m_connInfo.c_str());

  if (PQstatus(m_connection) != CONNECTION_OK) {
    logger->error("Connection to database failed: {}",
                  PQerrorMessage(m_connection));
    PQfinish(m_connection);
    m_connection = nullptr;
    return false;
  }

  logger->info("Connected to PostgreSQL database successfully.");
  return true;
}

bool Database::createUser(const Models::User &user) {
  if (!m_connection)
    throw std::runtime_error("Database not connected!");

  auto logger = Logger::getLogger();
  const char *paramValues[2];
  paramValues[0] = user.username.c_str();
  paramValues[1] = user.password.c_str();

  PGresult *res = PQexecParams(
      m_connection, "INSERT INTO users (username, password) VALUES ($1, $2)", 2,
      nullptr, paramValues, nullptr, nullptr, 0);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    logger->error("Error creating user: {}", PQerrorMessage(m_connection));
    PQclear(res);
    return false;
  }

  PQclear(res);
  logger->info("User created successfully: {}", user.username);
  return true;
}

Models::User Database::getUserById(int userId) {
  if (!m_connection)
    throw std::runtime_error("Database not connected!");

  auto logger = Logger::getLogger();

  std::stringstream ss;
  ss << userId;
  std::string idStr = ss.str();

  const char *paramValues[1] = {idStr.c_str()};
  PGresult *res = PQexecParams(
      m_connection, "SELECT id, username, password FROM users WHERE id = $1", 1,
      nullptr, paramValues, nullptr, nullptr, 0);

  Models::User user{};
  if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) == 1) {
    user.id = std::stoi(PQgetvalue(res, 0, 0));
    user.username = PQgetvalue(res, 0, 1);
    user.password = PQgetvalue(res, 0, 2);
  } else {
    logger->warn("No user found with id = {}", userId);
  }

  PQclear(res);
  return user;
}

std::vector<Models::User> Database::getAllUsers() {
  if (!m_connection)
    throw std::runtime_error("Database not connected!");

  auto logger = Logger::getLogger();
  PGresult *res =
      PQexec(m_connection, "SELECT id, username, password FROM users");
  std::vector<Models::User> users;

  if (PQresultStatus(res) == PGRES_TUPLES_OK) {
    int rows = PQntuples(res);
    for (int i = 0; i < rows; ++i) {
      Models::User user;
      user.id = std::stoi(PQgetvalue(res, i, 0));
      user.username = PQgetvalue(res, i, 1);
      user.password = PQgetvalue(res, i, 2);
      users.push_back(user);
    }
  } else {
    logger->error("Error retrieving users: {}", PQerrorMessage(m_connection));
  }

  PQclear(res);
  return users;
}

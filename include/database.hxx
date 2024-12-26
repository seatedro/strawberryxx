#ifndef DATABASE_HXX
#define DATABASE_HXX

#include <string>
#include <vector>

#include "models/chat.hxx"
#include "models/user.hxx"

struct pg_conn;

class Database {
public:
  Database(const std::string &conninfo);
  ~Database();

  bool connect();

  // CRUD for User
  bool createUser(const Models::User &user);
  Models::User getUserById(int userId);
  std::vector<Models::User> getAllUsers();

private:
  std::string m_connInfo;
  pg_conn *m_connection; // PGconn pointer
};

#endif // DATABASE_HXX

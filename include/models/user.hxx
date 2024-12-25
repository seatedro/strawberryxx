#ifndef USER_HXX
#define USER_HXX

#include <string>

namespace Models {

struct User {
  int id;
  std::string username;
  std::string password;
};

} // namespace Models

#endif // USER_HXX

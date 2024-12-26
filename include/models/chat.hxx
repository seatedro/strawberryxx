#ifndef CHAT_HXX
#define CHAT_HXX

#include <chrono>
#include <string>

namespace Models {

struct ChatEntry {
  int id;
  int userId; // foreign key to user
  std::string message;
  std::chrono::system_clock::time_point timestamp;
};

} // namespace Models

#endif // CHAT_HXX

#ifndef SESSION_MANAGER_HXX
#define SESSION_MANAGER_HXX

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

struct SessionData {
  int userId;
  std::chrono::system_clock::time_point expiry;
};

class SessionManager {
public:
  static SessionManager &instance();

  // Creates a new session for a user ID and returns the session token
  std::string createSession(int userId);

  // Validates a session token, returns userId if session is valid
  std::optional<int> validateSession(const std::string &token);

  // Invalidates/removes a session
  void invalidateSession(const std::string &token);

private:
  SessionManager() = default;

  // A simple in-memory map: token -> SessionData
  std::unordered_map<std::string, SessionData> sessions_;
  std::mutex mutex_;

  // Helper method to generate random token (fake, minimal example)
  std::string generateToken();
};

#endif // SESSION_MANAGER_HXX

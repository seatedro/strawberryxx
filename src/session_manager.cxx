#include "session_manager.hxx"
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <string>

SessionManager &SessionManager::instance() {
  static SessionManager inst;
  return inst;
}

std::string SessionManager::createSession(int userId) {
  std::lock_guard<std::mutex> lock(mutex_);

  std::string token = generateToken();

  // Set session expiry to, say, 1 hour from now
  auto now = std::chrono::system_clock::now();
  auto expiry = now + std::chrono::hours(1);

  sessions_[token] = SessionData{userId, expiry};

  return token;
}

std::optional<int> SessionManager::validateSession(const std::string &token) {
  std::lock_guard<std::mutex> lock(mutex_);

  auto it = sessions_.find(token);
  if (it != sessions_.end()) {
    auto now = std::chrono::system_clock::now();
    if (now < it->second.expiry) {
      // Session is valid
      return it->second.userId;
    } else {
      // Session expired, remove it
      sessions_.erase(it);
    }
  }
  return std::nullopt;
}

void SessionManager::invalidateSession(const std::string &token) {
  std::lock_guard<std::mutex> lock(mutex_);
  sessions_.erase(token);
}

std::string SessionManager::generateToken() {
  static std::mt19937_64 rng(std::random_device{}());
  static std::uniform_int_distribution<uint64_t> dist;

  uint64_t part1 = dist(rng);
  uint64_t part2 = dist(rng);

  std::stringstream ss;
  ss << std::hex << std::setw(16) << std::setfill('0') << part1 << std::setw(16)
     << std::setfill('0') << part2;

  return ss.str();
}

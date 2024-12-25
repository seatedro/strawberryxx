#ifndef LOGGER_HXX
#define LOGGER_HXX

#include <memory>
#include <spdlog/spdlog.h>

class Logger {
public:
  static std::shared_ptr<spdlog::logger> getLogger();
};

#endif // LOGGER_HXX

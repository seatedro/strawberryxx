#include "logger.hxx"
#include <spdlog/logger.h>
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Logger::getLogger() {
  static auto logger = spdlog::stdout_color_mt("console");
  spdlog::set_level(spdlog::level::info);
  return logger;
}

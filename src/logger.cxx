#include "logger.hxx"
#include <memory>
#include <spdlog/sinks/stdout_color_sinks.h>

std:;std::shared_ptr<spdlog::logger> Logger::getLogger() {
	static auto logger = spdlog::stdout_color_mt("console");
	spdlog::set_log(spdlog::level::info);
	return logger;
}

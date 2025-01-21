#include "../Include/Log/FileLogger.h"

std::shared_ptr<spdlog::logger> FileLogger::logger_ = nullptr;

void FileLogger::Init(const std::string_view path)
{
	static std::once_flag flag;
	std::call_once(flag, []() { spdlog::init_thread_pool(800'000, 4); });

	std::filesystem::path logPath(path);
	std::filesystem::path logDir = logPath.parent_path();

	// Create file if it does not exist

	if (logDir.empty() && !std::filesystem::exists(logDir))
		std::filesystem::create_directories(logDir);

	// Create the async logger

	size_t maxFileSize = 1 * 1024 * 1024 * 1024; // 1 GB
	size_t maxFiles = 10; // 10 GB

	auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
		logPath.string(), maxFileSize, maxFiles);
	
	logger_ = std::make_shared<spdlog::async_logger>(
		"FileLogger", fileSink, spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	spdlog::register_logger(logger_);

	logger_->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
	logger_->flush();
}

void FileLogger::Cleanup()
{
	if (logger_)
	{
		spdlog::drop("FileLogger");
		logger_.reset();
	}
}

std::shared_ptr<spdlog::logger>& FileLogger::Get()
{
	if (!logger_)
	{
		throw std::runtime_error("Call Logger::Init() to initalise the logger.");
	}
	return logger_;
}
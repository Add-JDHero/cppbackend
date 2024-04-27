#pragma once

#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <optional>
#include <mutex>
#include <optional>
#include <syncstream>
#include <thread>

using namespace std::literals;

static const std::string_view base_dir = "/usr/log";
using time_point = std::chrono::_V2::system_clock::time_point;

#define LOG(...) Logger::GetInstance().Log(__VA_ARGS__)

class Logger {
    time_point GetTime() const {
        if (manual_ts_) {
            return *manual_ts_;
        }

        return std::chrono::system_clock::now();
    }
    
    std::_Put_time<char> GetTimeStamp() const;

    // Для имени файла возьмите дату с форматом "%Y_%m_%d"
    std::string GetFileTimeStamp() const;

    void OpenLogFile();

    Logger() = default;
    Logger(const Logger&) = delete;

public:
    static Logger& GetInstance() {
        static Logger obj;
        return obj;
    }

    // Выведите в поток все аргументы.
    template<class... Ts>
    void Log(const Ts&... args) {
        OpenLogFile();
        std::osyncstream sync_stream(log_file_);
        auto time_stamp = GetTimeStamp();
        sync_stream << time_stamp << ": ";
        (sync_stream << ... << args) << std::endl;
    }

    // Установите manual_ts_. Учтите, что эта операция может выполняться
    // параллельно с выводом в поток, вам нужно предусмотреть 
    // синхронизацию.
    void SetTimestamp(std::chrono::system_clock::time_point ts);

private:
    std::ofstream log_file_;
    std::string current_date_;
    std::optional<std::chrono::system_clock::time_point> manual_ts_;
};

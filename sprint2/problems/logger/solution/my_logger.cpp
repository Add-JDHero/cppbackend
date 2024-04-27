#include "my_logger.h"

void Logger::SetTimestamp(std::chrono::system_clock::time_point ts) {
    manual_ts_ = ts;
}

void Logger::OpenLogFile() {
    auto new_date = GetFileTimeStamp();
    if (new_date != current_date_ || !log_file_.is_open()) {
        if (log_file_.is_open()) {
            log_file_.close();
        }
        std::string file_name = "/var/log/sample_log_" + new_date + ".log";
        log_file_.open(file_name, std::ofstream::out | std::ofstream::app);
        current_date_ = new_date;
    }
}

std::string Logger::GetFileTimeStamp() const {
    auto t_c = std::chrono::system_clock::to_time_t(GetTime());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t_c), "%Y_%m_%d");

    return ss.str();
}

std::_Put_time<char> Logger::GetTimeStamp() const {
    const auto now = GetTime();
    const auto t_c = std::chrono::system_clock::to_time_t(now);

    return std::put_time(std::localtime(&t_c), "%F %T");
}

/* time_point Logger::GetTime() const {
    if (manual_ts_) {
        return *manual_ts_;
    }

    return std::chrono::system_clock::now();
} */
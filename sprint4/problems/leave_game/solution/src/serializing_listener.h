#pragma once

#include "application.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <boost/json.hpp>

namespace sezialization {

using milliseconds = std::chrono::milliseconds;

class SerializingListener {
public:
    SerializingListener(app::Application& app, const std::string& state_file, milliseconds save_period);

    void OnTick(milliseconds delta);

    void SaveStateToFile();
    void LoadStateFromFile();

private:
    app::Application& app_;
    std::string state_file_;
    milliseconds save_period_;
    milliseconds time_since_last_save_{0};
};

} // namespace serialization


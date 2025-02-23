#include "serializing_listener.h"
#include <iostream>
#include <filesystem>

namespace serialization {

SerializingListener::SerializingListener(app::Application& app, 
                                         const std::string& state_file, 
                                         milliseconds save_period)
    : app_(app), state_file_(state_file), save_period_(save_period) {}

void SerializingListener::OnTick(milliseconds delta) {
    time_since_last_save_ += delta;

    if (time_since_last_save_ >= save_period_) {
        SaveStateToFile();
        time_since_last_save_ = milliseconds{0};
    }
}

void SerializingListener::SaveStateToFile() {
    try {
        boost::json::object state_json = app_.SerializeState();

        std::string temp_file = state_file_ + ".tmp";
        std::ofstream ofs(temp_file);
        if (!ofs) {
            throw std::runtime_error("Failed to open temporary state file for writing.");
        }

        ofs << boost::json::serialize(state_json);
        ofs.close();

        std::filesystem::rename(temp_file, state_file_);
        std::cout << "Game state saved to " << state_file_ << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error saving game state: " << e.what() << std::endl;
    }
}

void SerializingListener::LoadStateFromFile() {
    if (!std::filesystem::exists(state_file_)) {
        std::cout << "No previous state file found. Starting fresh." << std::endl;
        return;
    }

    try {
        std::ifstream ifs(state_file_);
        if (!ifs) {
            throw std::runtime_error("Failed to open state file for reading.");
        }

        std::stringstream buffer;
        buffer << ifs.rdbuf();
        ifs.close();

        boost::json::value state_json = boost::json::parse(buffer.str());
        app_.DeserializeState(state_json.as_object());

        std::cout << "Game state restored from " << state_file_ << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error loading game state: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

} // namespace serialization


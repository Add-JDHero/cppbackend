#pragma once
#include "model_serialization.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <filesystem>
#include <fstream>
#include <functional>

namespace infra {

class SerializingListener : public app::ApplicationListener {
public:
    SerializingListener(std::function<std::vector<serialization::GameSessionRepr>()> get_state,
                        std::function<void(const std::vector<serialization::GameSessionRepr>&)> load_state,
                        const std::string& state_file,
                        app::milliseconds save_period);

    void OnTick(app::milliseconds delta) override;
    void SaveStateToFile();
    void LoadStateFromFile();

private:
    std::function<std::vector<serialization::GameSessionRepr>()> get_state_;
    std::function<void(const std::vector<serialization::GameSessionRepr>&)> load_state_;
    std::string state_file_;
    app::milliseconds save_period_;
    app::milliseconds time_since_last_save_{0};
};

}

#include <chrono>
#include <ratio>

class ApplicationListener {
public:
    virtual void OnTick(std::chrono::milliseconds delta) = 0;
    virtual void LoadStateFromFile() = 0;
};

/* class SerializingListener : public ApplicationListener {
public:
    SerializingListener();

    void OnTick(std::chrono::milliseconds delta) override;
}; */
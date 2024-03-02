#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;
using Timer = net::steady_timer;
using namespace std::literals::chrono_literals;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;
using Handler = std::function<void()>;

using Strand = net::strand<net::io_context::executor_type>;

class Order : public std::enable_shared_from_this<Order> {
public:
    explicit Order(net::io_context& io, Store& store, Strand& strand) 
        : io_(io)
        , store_(store)
        , strand_(strand) {
    }

    void MakeHotDog(GasCooker& cooker, HotDogHandler handler) {
        auto bread = store_.GetBread();
        auto sausage = store_.GetSausage();

        BakeBread(cooker, bread, [self = shared_from_this(), this, handler, bread, sausage]() {
            if (++ingredients_ready_ == 2) {
                CompleteOrder(handler, bread, sausage);
            }
        });

        FrySausage(cooker,sausage, [self = shared_from_this(),this, handler, bread, sausage]() {
            if (++ingredients_ready_ == 2) {
                CompleteOrder(handler, bread, sausage);
            }
        });
        
    }

    void BakeBread(GasCooker& cooker, std::shared_ptr<Bread> bread, Handler handler) {
        bread->StartBake(cooker, [self = shared_from_this(), this, &bread, &handler]() {
            std::cout << "Timer begin\n";
            baking_timer_.async_wait(net::bind_executor(strand_, [&bread, &self, &handler](sys::error_code ec) {
                if (!ec) {
                    bread->StopBake();
                    handler();
                }
            }));
        });
    }

    void FrySausage(GasCooker& cooker, std::shared_ptr<Sausage> sausage, Handler handler) {
        sausage->StartFry(cooker, [self = shared_from_this(), this, &sausage, &handler]() {
            frying_timer_.async_wait(net::bind_executor(strand_, [&sausage, &self, &handler](sys::error_code ec) {
                if (!ec) {
                    sausage->StopFry();
                    handler();
                }
            }));
        });
    }

private:
    int GenerateId() {
        std::lock_guard<std::mutex> lock(id_mutex_); // Захватываем мьютекс для безопасного доступа к next_id_
        return ++next_id_;
    }

    void CompleteOrder(HotDogHandler handler, std::shared_ptr<Bread> bread, std::shared_ptr<Sausage> sausage) {
        auto hotDog = std::make_shared<HotDog>(GenerateId(), sausage, bread);
        handler(Result<HotDog>{*hotDog});
    }

    int ingredients_ready_ = 0;
    net::io_context& io_;
    Store& store_;
    Strand& strand_;
    std::mutex id_mutex_;
    int next_id_ = 0;
    Timer baking_timer_{io_, 1s};
    Timer frying_timer_{io_, 1s + 500ms};
};

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
    }

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        std::make_shared<Order>(io_, store_, strand_)->MakeHotDog(*gas_cooker_, std::move(handler));
    }

private:
    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    Strand strand_{net::make_strand(io_)};
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};

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

using namespace std::chrono;

namespace net = boost::asio;
using Timer = net::steady_timer;
using namespace std::literals::chrono_literals;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;
using Handler = std::function<void()>;

using Strand = net::strand<net::io_context::executor_type>;

class Order : public std::enable_shared_from_this<Order> {
public:
    explicit Order(net::io_context& io, Store& store
                , Strand& strand, HotDogHandler handler, std::shared_ptr<GasCooker> cooker, int order_id) 
        : io_(io)
        , store_(store)
        , strand_(strand)
        , baking_timer_(io)
        , frying_timer_(io)
        , order_id_(order_id)
        , hotdog_handler_(std::move(handler)) 
        , gas_cooker_(std::move(cooker)) {
    }


    void MakeHotDog() {
        auto bread = store_.GetBread();
        auto sausage = store_.GetSausage();

        BakeBread(bread, [self = shared_from_this(), this, bread, sausage]() {
            if (++ingredients_ready_ == 2) {
                CompleteOrder(bread, sausage);
            }
        });

        FrySausage(sausage, [self = shared_from_this(),this, bread, sausage]() {
            if (++ingredients_ready_ == 2) {
                CompleteOrder(bread, sausage);
            }
        });
        
    }

    void BakeBread(std::shared_ptr<Bread> bread, Handler handler) {
        net::post(strand_, [this, bread, handler = std::move(handler)]{
            bread->StartBake(*gas_cooker_, [self = shared_from_this(), handler = std::move(handler), this, bread]() {
                baking_timer_.expires_after(1000ms);
                baking_timer_.async_wait(net::bind_executor(strand_, [bread, self, handler = std::move(handler)](sys::error_code ec) {
                    if (!ec) {
                        bread->StopBake();
                        handler();
                    } else {
                        std::cout << ec.what();
                    }
                }));
            });
        });
    }

    void FrySausage(std::shared_ptr<Sausage> sausage, Handler handler) {
        net::post(strand_, [this, sausage, handler = std::move(handler)]{
            sausage->StartFry(*gas_cooker_, [self = shared_from_this(), handler = std::move(handler), this, sausage]() {
                frying_timer_.expires_after(1500ms);
                frying_timer_.async_wait(net::bind_executor(strand_, [handler = std::move(handler), sausage, self](sys::error_code ec) {
                    if (!ec) {
                        sausage->StopFry();
                        handler();
                    }
                }));
            });
        });
    }

private: 
    void CompleteOrder(std::shared_ptr<Bread> bread, std::shared_ptr<Sausage> sausage) {
        auto hotDog = std::make_shared<HotDog>(order_id_, sausage, bread);
        hotdog_handler_(Result<HotDog>{*hotDog});
    }

    HotDogHandler hotdog_handler_;
    int ingredients_ready_ = 0;
    net::io_context& io_;
    Store& store_;
    Strand& strand_;
    int order_id_;
    std::shared_ptr<GasCooker> gas_cooker_;
    std::mutex id_mutex_;
    Timer baking_timer_;
    Timer frying_timer_;
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
        net::dispatch(strand_, [handler = std::move(handler), this]{
            std::make_shared<Order>(io_, store_, strand_, std::move(handler), gas_cooker_, GenerateId())->MakeHotDog();
        });
    }

private:
    int GenerateId() {
        std::lock_guard<std::mutex> lock(id_mutex_);

        return ++next_id_;
    }

    net::io_context& io_;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    Strand strand_{net::make_strand(io_)};
    int next_id_ = 0;
    std::mutex id_mutex_;
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};

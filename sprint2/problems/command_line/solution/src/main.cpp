#include "sdk.h"
#include "application.h"

#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>
#include <boost/asio/signal_set.hpp>

#include "command_line_parser.h"
#include "util_tests.h"
#include "json_loader.h"
#include "log.h"
#include "request_handler.h"
#include "ticker.h"

// #define TESTS

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {
// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char* argv[]) {

    Args arg;

    try {
        if (auto args = ParseCommandLine(argc, argv)) {
            arg = *args;
            } else {
            return EXIT_FAILURE;
            }
        } catch (const std::exception& e) {
            std::cout << "Parse arguments failure. " << e.what() << std::endl;
            return EXIT_FAILURE;
    }

    #ifdef TESTS 
        Tests::Tests();
    #endif

    try {
        SetupLogging();

        // 2. Инициализируем io_context
        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);
        net::strand strand = net::make_strand(ioc);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                std::cout << "Signal "sv << signal_number << " received"sv << std::endl;
                ioc.stop();
            }
        });

        // model::Game game = json_loader::LoadGame(arg.config);
        app::Application app(arg.config);
        // app::Application app(game);

        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto handler = std::make_shared<http_handler::RequestHandler>(strand, arg.www_root, app);
        http_handler::LoggingRequestHandler logging_handler(handler);

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto&& req, auto&& send) {
            logging_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        // Cообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        ServerStartLog(port, address);

        if (arg.period) {
            app.SetManualTicker(false);
            auto ticker = std::make_shared<game_time::Ticker>(strand, 10ms,
                [&app](std::chrono::milliseconds delta) { app.Tick(double(delta.count()) / double(1000)); }
            );

            ticker->Start();
        }

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run(); 
        });

    } catch (const std::exception& ex) {
        ServerStopLog(EXIT_FAILURE, ex.what());

        return EXIT_FAILURE;
    }
}
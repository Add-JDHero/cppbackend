#include "model.h"
#include "sdk.h"
#include "application.h"
#include "command_line_parser.h"
#include "util_tests.h"
#include "json_loader.h"
#include "log.h"
#include "tagged.h"
#include "request_handler.h"
#include "ticker.h"
#include "extra_data.h"

#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>
#include <boost/asio/signal_set.hpp>


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

model::Game::MapIdToLootTypesCount ExtractLootTypes(MapLootTypes& loot_types) {
    model::Game::MapIdToLootTypesCount result;

    for (const auto& [map_id, json_array] : loot_types.mapId_to_lootTypes) {
        int loot_count = json_array->size();

        result[map_id] = loot_count;
    }

    return result;
}

}  // namespace

    // =================================================================
    // TODO: перенести game в Application (ломается перемещение собаки!) 
    // =================================================================

int main(int argc, const char* argv[]) {

    #ifdef TESTS 
        Tests::Tests();
    #endif

    try {
        SetupLogging();

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

        double tick_time = static_cast<double>(arg.period) / static_cast<double>(1000);

        // 1. Загружаем карту из файла и построить модель игры
        model::Game game = json_loader::LoadGame(arg.config);

        json::value config = 
            json_loader::ParseConfigFile(util::ReadFromFileIntoString(arg.config));

        MapLootTypes loot_types = json_loader::ParseLootTypes(config);

        game.SetDefaultTickTime(tick_time);
        game.ConfigureLootTypesToMaps(ExtractLootTypes(loot_types));

        // model::GameSession::SetDefaultTickTime(tick_time);
        app::Application app(game);

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


        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto handler = 
            std::make_shared<http_handler::RequestHandler>(game, strand, arg.www_root, app, loot_types);
        http_handler::LoggingRequestHandler logging_handler(handler);

        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto&& req, auto&& send) {
            logging_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        // Cообщает тестам о том, что сервер запущен и готов обрабатывать запросы
        ServerStartLog(port, address);


        auto ms = std::chrono::milliseconds(static_cast<int>(arg.period));
        auto ticker = 
            std::make_shared<game_time::Ticker>(strand, ms,
            [&game](std::chrono::milliseconds delta) { 
                game.Tick(static_cast<double>(delta.count()) / static_cast<double>(1000)); 
            }
        );
        ticker->Start();

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run(); 
        });

    } catch (const std::exception& ex) {
        ServerStopLog(EXIT_FAILURE, ex.what());

        return EXIT_FAILURE;
    }
}
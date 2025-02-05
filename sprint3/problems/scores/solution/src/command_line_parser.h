#pragma once 

#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <optional>
#include <vector>

using namespace std::literals;

const unsigned int DEFAULT_TICK_PERIOD = 10;

struct Args {
    unsigned int period = DEFAULT_TICK_PERIOD;
    std::string config;
    std::string www_root;
    bool random;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    po::options_description desc{"All options"s};
    Args args;

    // Allowed options:
    // -h [ --help ]                     produce help message
    // -t [ --tick-period ] milliseconds set tick period
    // -c [ --config-file ] file         set config file path
    // -w [ --www-root ] dir             set static files root
    // --randomize-spawn-points          spawn dogs at random positions
    desc.add_options()                                                                                           //
        ("help,h", "produce help message")                                                                       //
        ("tick-period,t", po::value<unsigned int>(&args.period)->value_name("milliseconds"), "set tick period")  //
        ("config-file,c", po::value(&args.config)->value_name("file"), "set config file path")                   //
        ("www-root,w", po::value(&args.www_root)->value_name("dir"), "set static files root")                    //
        ("randomize-spawn-points", po::value<bool>(&args.random), "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc;
        return std::nullopt;
    }

    if (vm.contains("config-file") && vm.contains("www-root")) {
        return args;
    } else {
        throw std::runtime_error("Usage: game_server --config-file <game-config-json> --www-root <dir-to-content>");
    }

    return std::nullopt;
}

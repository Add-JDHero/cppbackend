#include "audio.h"
#include <boost/asio.hpp>
#include <iostream>

namespace net = boost::asio;
using net::ip::udp;
using namespace std::literals;

static const int port = 1234;
static const int MAX_FRAMES = 65000;

static const size_t max_buffer_size = 1024;

void StartServer(uint16_t port) {
    Player player(ma_format_u8, 1);
    typedef std::chrono::duration<float> fsec;

    try {
        boost::asio::io_context io_context;

        udp::socket socket(io_context, udp::endpoint(udp::v4(), port));

        int i = 0;
        for (;;) {
            std::array<char, max_buffer_size> recv_buf;
            udp::endpoint remote_endpoint;

            // Получаем не только данные, но и endpoint клиента
            auto size = socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

            std::cout << "Message number: " << i++ << " recieved\n";
            player.PlayBuffer(recv_buf.data(), recv_buf.size(), 1.5s); 
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return ;
}

Recorder::RecordingResult Record() {
    Recorder recorder(ma_format_u8, 1);

    return recorder.Record(MAX_FRAMES, 1.5s);
}

void StartClient(uint16_t port) {
    Recorder recorder(ma_format_u8, 1);
    try {
        net::io_context io_context;

        // Перед отправкой данных нужно открыть сокет. 
        // При открытии указываем протокол (IPv4 или IPv6) вместо endpoint.
        udp::socket socket(io_context, udp::v4());

        while (true) {
            std::cout << "Write destination IP" << std::endl;
            std::string address_str;
            std::getline(std::cin, address_str);

            boost::system::error_code ec;
            auto endpoint = udp::endpoint(net::ip::make_address(address_str, ec), port);

            auto rec_result = Record();

            socket.send_to(net::buffer(rec_result.data, rec_result.frames * recorder.GetFrameSize()), endpoint);
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return ;
}

int main(int argc, char** argv) {
    Recorder recorder(ma_format_u8, 1);
    Player player(ma_format_u8, 1);

    if (argv[1] == "server"sv) {
        StartServer(port);
    } else {
        StartClient(port);
    }

    /* while (true) {
        std::string str;

        std::cout << "Press Enter to record message..." << std::endl;
        std::getline(std::cin, str);

        auto rec_result = recorder.Record(65000, 1.5s);
        std::cout << "Recording done" << std::endl;

        player.PlayBuffer(rec_result.data.data(), rec_result.frames, 1.5s);
        std::cout << "Playing done" << std::endl;
    } */

    return 0;
}

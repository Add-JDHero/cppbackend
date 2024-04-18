#include "util.h"

namespace util {
    namespace beast = boost::beast;
    namespace sys = boost::system;
    namespace http = beast::http;
    namespace net = boost::asio;

    class SyncWriteOStreamAdapter {
    public:
        explicit SyncWriteOStreamAdapter(std::ostream& os)
            : os_{os} {
        }

        template <typename ConstBufferSequence>
        size_t write_some(const ConstBufferSequence& cbs, sys::error_code& ec) {
            const size_t total_size = net::buffer_size(cbs);
            if (total_size == 0) {
                ec = {};
                return 0;
            }
            size_t bytes_written = 0;
            for (const auto& cb : cbs) {
                const size_t size = cb.size();
                const char* const data = reinterpret_cast<const char*>(cb.data());
                if (size > 0) {
                    if (!os_.write(reinterpret_cast<const char*>(data), size)) {
                        ec = make_error_code(boost::system::errc::io_error);
                        return bytes_written;
                    }
                    bytes_written += size;
                }
            }
            ec = {};
            return bytes_written;
        }

        template <typename ConstBufferSequence>
        size_t write_some(const ConstBufferSequence& cbs) {
            sys::error_code ec;
            const size_t bytes_written = write_some(cbs, ec);
            if (ec) {
                throw std::runtime_error("Failed to write");
            }
            return bytes_written;
        }

    private:
        std::ostream& os_;
    };

    std::string ReadFromFileIntoString(const std::filesystem::path& file_path) {
        std::ifstream config_file(file_path, std::ios::in | std::ios::binary);

        if (!config_file.is_open()) {
            throw std::runtime_error("Failed to open file: " + file_path.string());
        }

        char buff[BUFF_SIZE];
        std::string str;
        std::streamsize count = 0;
        while ( config_file.read(buff, BUFF_SIZE) || (count = config_file.gcount()) ) {
            str.append(buff, count);
        }

        return str;
    }

    http::response<http::file_body> ReadStaticFile(std::string_view file_name) {
        http::response<http::file_body> res;
        res.version(11);  // HTTP/1.1
        res.result(http::status::ok);
        const beast::string_view text = "text/plain";
        res.insert(http::field::content_type, text);

        http::file_body::value_type file;

        if (sys::error_code ec; file.open(file_name, beast::file_mode::read, ec), ec) {
            throw std::runtime_error("Failed to open file: " + file_name.string());
        }

        res.body() = std::move(file);
        // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
        // в зависимости от свойств тела сообщения
        res.prepare_payload();

        std::stringstream ss;

        SyncWriteOStreamAdapter adapter{ss};

        http::write(adapter, res);
        
        std::cout << ss.str();
        
        return {};
    }
}
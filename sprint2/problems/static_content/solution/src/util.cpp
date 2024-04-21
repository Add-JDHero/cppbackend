#include "util.h"
#include <filesystem>

namespace util {
    namespace beast = boost::beast;
    namespace sys = boost::system;
    namespace http = beast::http;
    namespace net = boost::asio;

    using namespace std::literals;

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

    beast::string_view MimeType(beast::string_view path) {
        using beast::iequals;
        auto const ext = [&path]{
            auto const pos = path.rfind(".");
            if(pos == beast::string_view::npos)
                return beast::string_view{};
            return path.substr(pos);
        }();
        if (iequals(ext, ".htm"))  return "text/html";
        if (iequals(ext, ".html")) return "text/html";
        // if(iequals(ext, ".php"))  return "text/html";
        if (iequals(ext, ".css"))  return "text/css";
        if (iequals(ext, ".txt"))  return "text/plain";
        if (iequals(ext, ".js"))   return "text/javascript";
        if (iequals(ext, ".json")) return "application/json";
        if (iequals(ext, ".xml"))  return "application/xml";
        // if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
        // if (iequals(ext, ".flv"))  return "video/x-flv";
        if (iequals(ext, ".png"))  return "image/png";
        if (iequals(ext, ".jpe"))  return "image/jpeg";
        if (iequals(ext, ".jpeg")) return "image/jpeg";
        if (iequals(ext, ".jpg"))  return "image/jpeg";
        if (iequals(ext, ".gif"))  return "image/gif";
        if (iequals(ext, ".bmp"))  return "image/bmp";
        if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
        if (iequals(ext, ".tiff")) return "image/tiff";
        if (iequals(ext, ".tif"))  return "image/tiff";
        if (iequals(ext, ".svg"))  return "image/svg+xml";
        if (iequals(ext, ".svgz")) return "image/svg+xml";
        if (iequals(ext, ".mp3")) return "audio/mpeg";
        return "application/octet-stream";
    }

    std::string ExtractFileExtension(const std::filesystem::path& path) {
        auto path_sv = std::string_view(path.c_str());
        if (path_sv.find_last_of('.') != std::string_view::npos) {
            std::string_view ext = path_sv.substr(path_sv.find_last_of('.'), std::string_view::npos);
            return std::string(ext);
        }
        
        return {};
    }

    std::string ReadFromFileIntoString(const std::filesystem::path& file_path) {
        std::ifstream config_file(file_path, std::ios::in | std::ios::binary);

        if (!config_file.is_open()) {
            throw std::runtime_error("Failed to open file: " + file_path.string());
        }

        char buff[BUFF_SIZE];
        std::string str = "";
        std::streamsize count = 0;
        while ( config_file.read(buff, BUFF_SIZE) || (count = config_file.gcount()) ) {
            str.append(buff, count);
        }

        return str;
    }

    std::string TestFunc(const std::filesystem::path& file_path) {
        // std::ifstream config_file(file_path, std::ios::in | std::ios::binary);

        // if (!config_file.is_open()) {
        //     throw std::runtime_error("Failed to open file: " + file_path.string());
        // }

        // char buff[BUFF_SIZE];
        // std::string str = "";
        // std::streamsize count = 0;
        // while ( config_file.read(buff, BUFF_SIZE) || (count = config_file.gcount()) ) {
        //     str.append(buff, count);
        // }

        // return str;
        return {};
    }

    http::response<http::file_body> ReadStaticFile(const std::filesystem::path& file_path) {
        http::response<http::file_body> res;
        res.version(11);  // HTTP/1.1
        res.result(http::status::ok);
        auto res_extention = MimeType(ExtractFileExtension(file_path));
        res.insert(http::field::content_type, res_extention);

        http::file_body::value_type file;

        if (!std::filesystem::exists(file_path)) {
            throw;
        }

        if (sys::error_code ec; file.open(file_path.c_str(), beast::file_mode::read, ec), ec) {
            throw std::runtime_error("Failed to open file: " + file_path.string());
        }

        res.body() = std::move(file);
        // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
        // в зависимости от свойств тела сообщения
        // res.prepare_payload();

        return res;
    }
}
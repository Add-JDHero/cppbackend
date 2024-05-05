#include "log.h"


void SetupLogging() {
    logging::add_common_attributes();

    logging::add_console_log(
        std::clog,
        keywords::format = &JsonFormatter
    );
}

void JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    boost::json::object log_entry;
    auto ts = *rec[timestamp];

    log_entry["timestamp"] = to_iso_extended_string(ts);
    log_entry["message"] = *rec[expr::smessage];
    log_entry["data"] = *rec[additional_data];

    strm << boost::json::serialize(log_entry);
}

void ServerStartLog(unsigned port, boost::asio::ip::address ip) {
    boost::json::object data;

    data["port"] = port;
    data["address"] = ip.to_string();

    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << "server started";
}

void ServerStopLog(unsigned err_code, std::string_view ex) {
    boost::json::object data;

    data["code"] = err_code;
    if (ex != "") {
        data["exception"] = std::string(ex);
    }
   
    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << "server exited";
}

void ServerErrorLog(unsigned err_code, std::string_view message, std::string_view place) {
    boost::json::object data;
    
    data["coode"] = err_code;
    data["text"] = std::string(message);
    data["where"] = std::string(place);

    BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, data) << "error";
}
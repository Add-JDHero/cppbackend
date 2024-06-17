#pragma once

#include "sdk.h"
#include <boost/asio/ip/address.hpp>
#include <boost/log/utility/setup/common_attributes.hpp> 
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/trivial.hpp>
#include <boost/date_time.hpp>
#include <string_view>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/json.hpp>

namespace logging = boost::log;

BOOST_LOG_ATTRIBUTE_KEYWORD(additional_data, "AdditionalData", boost::json::value)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

void SetupLogging();

void JsonFormatter(logging::record_view const& rec, logging::formatting_ostream& strm);

void ServerStartLog(unsigned port, boost::asio::ip::address ip);
void ServerStopLog(unsigned err_code, std::string_view ex);
void ServerErrorLog(unsigned err_code, std::string_view message, std::string_view place);

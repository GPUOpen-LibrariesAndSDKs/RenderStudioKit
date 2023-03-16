#pragma once

#define _FUNCTION_NAME __FUNCTION__
#pragma warning(push, 0)
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#pragma warning(pop)

namespace RenderStudio
{

using RenderStudioLogger = boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>;
BOOST_LOG_GLOBAL_LOGGER(logger, RenderStudioLogger)

} // namespace RenderStudio

#define _LOG_SET(_severity) \
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::_severity)
#define LOG_SET_TRACE() _LOG_SET(trace)
#define LOG_SET_DEBUG() _LOG_SET(debug)
#define LOG_SET_INFO() _LOG_SET(info)
#define LOG_SET_WARNING() _LOG_SET(warning)
#define LOG_SET_ERROR() _LOG_SET(error)
#define LOG_SET_FATAL() _LOG_SET(fatal)

#define _LOG(_severity)                                                        \
    BOOST_LOG_SEV(RenderStudio::logger::get(), boost::log::trivial::_severity) \
        << boost::log::add_value("Function", _FUNCTION_NAME)                   \
        << boost::log::add_value("Line", static_cast<unsigned long>(__LINE__))
#define LOG_TRACE _LOG(trace)
#define LOG_DEBUG _LOG(debug)
#define LOG_INFO _LOG(info)
#define LOG_WARNING _LOG(warning)
#define LOG_ERROR _LOG(error)
#define LOG_FATAL _LOG(fatal)

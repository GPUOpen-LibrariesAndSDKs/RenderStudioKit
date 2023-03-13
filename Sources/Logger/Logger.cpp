#include "Logger.h"

#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/block_on_overflow.hpp>
#include <boost/log/sinks/bounded_fifo_queue.hpp>
#include <boost/log/sinks/bounded_ordering_queue.hpp>
#include <boost/log/sinks/drop_on_overflow.hpp>
#include <boost/log/sinks/unbounded_fifo_queue.hpp>
#include <boost/log/sinks/unbounded_ordering_queue.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/console.hpp>

#define FMT_HEADER_ONLY
#include <fmt/color.h>
#include <fmt/format-inl.h>

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;

namespace RenderStudio
{

void
coloring_formatter(logging::record_view const& rec, logging::formatting_ostream& strm)
{
    // Serverity section
    auto severity = rec[logging::trivial::severity];

    if (severity)
    {
        // Set the color
        switch (severity.get())
        {
        case logging::trivial::severity_level::trace:
            strm << "trace";
            break;
        case logging::trivial::severity_level::debug:
            strm << "debug";
            break;
        case logging::trivial::severity_level::info:
            strm << "info ";
            break;
        case logging::trivial::severity_level::warning:
            strm << "warn ";
            break;
        case logging::trivial::severity_level::error:
            strm << "error";
            break;
        case logging::trivial::severity_level::fatal:
            strm << "fatal";
            break;
        default:
            break;
        }
    }

    // Message section
    strm << " | " << rec[expr::smessage] << " ";
    // Function and line section
    strm << "(" << logging::extract<std::string>("Function", rec).get() << ":"
         << std::to_string(logging::extract<unsigned long>("Line", rec).get()) << ")";
}

BOOST_LOG_GLOBAL_LOGGER_INIT(logger, RenderStudioLogger)
{
    logging::add_common_attributes();

    boost::shared_ptr<sinks::text_ostream_backend> backend = boost::make_shared<sinks::text_ostream_backend>();
    backend->add_stream(boost::shared_ptr<std::ostream>(&std::cout, boost::null_deleter()));
    backend->auto_flush(true);
    using sink_t = sinks::asynchronous_sink<sinks::text_ostream_backend>;
    boost::shared_ptr<sink_t> sink(new sink_t(backend));
    sink->set_formatter(&coloring_formatter);
    boost::log::core::get()->add_sink(sink);

    RenderStudioLogger logger;
    return logger;
}

} // namespace RenderStudio
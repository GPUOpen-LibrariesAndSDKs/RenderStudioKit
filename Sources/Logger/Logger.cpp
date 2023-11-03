// Copyright 2023 Advanced Micro Devices, Inc
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Logger.h"

#pragma warning(push, 0)
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
#include <boost/make_shared.hpp>
#include <fmt/color.h>
#include <fmt/format.h>
#pragma warning(pop)

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;

namespace RenderStudio
{

void
coloring_formatter(logging::record_view const& rec, logging::wformatting_ostream& strm)
{
    // Serverity section
    auto severity = rec[logging::trivial::severity];

    if (severity)
    {
        // Set the color
        switch (severity.get())
        {
        case logging::trivial::severity_level::trace:
            strm << fmt::format(fg(fmt::color::light_steel_blue), "[studio]");
            strm << " 🕵️‍♂️ ";
            break;
        case logging::trivial::severity_level::debug:
            strm << fmt::format(fg(fmt::color::cornflower_blue), "[studio]");
            strm << " 🔍 ";
            break;
        case logging::trivial::severity_level::info:
            strm << fmt::format(fg(fmt::color::light_green), "[studio]");
            strm << " ✅ ";
            break;
        case logging::trivial::severity_level::warning:
            strm << fmt::format(fg(fmt::color::yellow), "[studio]");
            strm << fmt::format(fg(fmt::color::yellow) | fmt::emphasis::bold, " ⚠️ ");
            break;
        case logging::trivial::severity_level::error:
            strm << fmt::format(fg(fmt::color::orange_red), "[studio]");
            strm << " ❌ ";
            break;
        case logging::trivial::severity_level::fatal:
            strm << fmt::format(fg(fmt::color::red), "[studio]");
            strm << " ⛔ ";
            break;
        default:
            break;
        }
    }

    // Message section
    strm << rec[expr::smessage] << " ";

    // Function and line section
    std::string function = "(" + logging::extract<std::string>("Function", rec).get() + ":"
        + std::to_string(logging::extract<unsigned long>("Line", rec).get()) + ")";
    strm << fmt::format(fg(fmt::color::dim_gray), function);
}

BOOST_LOG_GLOBAL_LOGGER_INIT(logger, RenderStudioLogger)
{
    logging::add_common_attributes();

    // Shared pointer for the backend
    auto backend = boost::make_shared<sinks::wtext_ostream_backend>();
    backend->add_stream(boost::shared_ptr<std::wostream>(&std::wcout, boost::null_deleter()));
    backend->auto_flush(true);

    // Create the sink
    using sink_t = sinks::asynchronous_sink<sinks::wtext_ostream_backend>;
    auto sink = boost::make_shared<sink_t>(backend);
    sink->set_formatter(&coloring_formatter);
    boost::log::core::get()->add_sink(sink);

    RenderStudioLogger logger;
    return logger;
}

} // namespace RenderStudio

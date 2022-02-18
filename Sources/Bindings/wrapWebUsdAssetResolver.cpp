#include "../Resolver.h"
#include "BoostIncludeWrapper.h"
#include <pxr/pxr.h>

#include BOOST_INCLUDE(python/class.hpp)

using namespace BOOST_NAMESPACE::python;

PXR_NAMESPACE_USING_DIRECTIVE

void
wrapWebUsdAssetResolver()
{
    using This = WebUsdAssetResolver;

    class_<This, bases<ArResolver>, BOOST_NAMESPACE::noncopyable>
        ("WebUsdAssetResolver", no_init)
        .def("SetRemoteServerAddress", &This::SetRemoteServerAddress, args("host", "port"))
        .staticmethod("SetRemoteServerAddress")
        ;
}

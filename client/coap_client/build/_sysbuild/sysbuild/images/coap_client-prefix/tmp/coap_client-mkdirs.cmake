# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/daniel/ncs/330rc2/nrf/samples/openthread/coap_client")
  file(MAKE_DIRECTORY "/home/daniel/ncs/330rc2/nrf/samples/openthread/coap_client")
endif()
file(MAKE_DIRECTORY
  "/home/daniel/ncs/330rc2/nrf/samples/openthread/coap_client/build/coap_client"
  "/home/daniel/ncs/330rc2/nrf/samples/openthread/coap_client/build/_sysbuild/sysbuild/images/coap_client-prefix"
  "/home/daniel/ncs/330rc2/nrf/samples/openthread/coap_client/build/_sysbuild/sysbuild/images/coap_client-prefix/tmp"
  "/home/daniel/ncs/330rc2/nrf/samples/openthread/coap_client/build/_sysbuild/sysbuild/images/coap_client-prefix/src/coap_client-stamp"
  "/home/daniel/ncs/330rc2/nrf/samples/openthread/coap_client/build/_sysbuild/sysbuild/images/coap_client-prefix/src"
  "/home/daniel/ncs/330rc2/nrf/samples/openthread/coap_client/build/_sysbuild/sysbuild/images/coap_client-prefix/src/coap_client-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/daniel/ncs/330rc2/nrf/samples/openthread/coap_client/build/_sysbuild/sysbuild/images/coap_client-prefix/src/coap_client-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/daniel/ncs/330rc2/nrf/samples/openthread/coap_client/build/_sysbuild/sysbuild/images/coap_client-prefix/src/coap_client-stamp${cfgdir}") # cfgdir has leading slash
endif()

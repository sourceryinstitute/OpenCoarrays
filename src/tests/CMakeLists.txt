if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  option(HIGH_RESOLUTION_TIMER "use the walltime.o wallclock timer" FALSE)
  option(LEGACY_ARCHITECTURE "use the SSE-enabled FFT library" FALSE)
endif()

if (HIGH_RESOLUTION_TIMER)
  message(WARNING
    "walltime.o clicks once per clock cycle on an x86 CPU with a 3.6 GHz clock.  "
    "Because modern processors can throttle their clock spees, do not "
    "rely on this timer for an absolute value, but it can be useful for "
    "comparison or relative execution execution times on the same platform."
    )
endif()

add_subdirectory(integration)
add_subdirectory(unit)
#add_subdirectory(performance)

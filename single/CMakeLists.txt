add_library(caf_single single.c ../common/caf_auxiliary.c)
target_compile_options(caf_single INTERFACE -fcoarray=lib)
install(TARGETS caf_single EXPORT OpenCoarraysTargets
  ARCHIVE DESTINATION lib
)

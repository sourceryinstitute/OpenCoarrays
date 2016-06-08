# Build the Open Fortran Parser parse table
# shellcheck disable=SC2154
function build_parse_table()
{
  info "Building parse table"
  pushd "${build_path}/ofp-sdf/fortran/syntax"
  info "Build command: make SDF2_PATH=\"${SDF2_PATH}\" ST_PATH=\"${ST_PATH}\" DYLD_LIBRARY_PATH=\"${DYLD_LIBRARY_PATH}\""
  make SDF2_PATH="${SDF2_PATH}" ST_PATH="${ST_PATH}" DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}"
  popd
  pushd "${build_path}/ofp-sdf/fortran/trans"
  make
  popd
}

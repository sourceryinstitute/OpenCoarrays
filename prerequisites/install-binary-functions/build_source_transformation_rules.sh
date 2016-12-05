# Build the Open Fortran Parser source transformation rules for coarray Fortran
# shellcheck disable=??????
function build_source_transformation_rules()
{
  info "Building source transformation rules"
  pushd "${OPENCOARRAYS_SRC_DIR}"/3rd-party-tools/open-fortran-parser-sdf/trans/
  info "Build command: make OFP_HOME=${ofp_build_dir} fast-to-ocaf"
  make OFP_HOME=${ofp_build_dir} fast-to-ocaf
  popd
}

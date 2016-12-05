# shellcheck disable=??????
function install_transpiler()
{
  ofp_install_dir="${install_path}"/transpiler 
  ofp_build_dir="${ofp_build_prefix}"/ofp-sdf
  info "Installing the Open Fortran Parser (ofp-sdf) in ${ofp_install_dir}" 
  mkdir -p "${ofp_install_dir}"
  cp "${ofp_build_dir}"/fortran/pp/Fortran.pp      "${ofp_install_dir}"
  cp "${ofp_build_dir}"/fortran/trans/fast2pp      "${ofp_install_dir}"
  cp "${ofp_build_dir}"/fortran/trans/ofp2fast     "${ofp_install_dir}"
  cp "${ofp_build_dir}"/fortran/syntax/Fortran.tbl "${ofp_install_dir}"
  pushd "${OPENCOARRAYS_SRC_DIR}"/3rd-party-tools/open-fortran-parser-sdf/trans/
  make OFP_HOME=${ofp_build_dir} fast-to-ocaf
  cp fast-to-ocaf "${ofp_install_dir}"
  popd
}

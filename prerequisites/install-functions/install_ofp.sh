# shellcheck disable=??????
function install_ofp()
{
  ofp_install_dir="${install_path}"/transpiler 
  info "Installing the Open Fortran Parser (ofp-sdf) in ${ofp_install_dir}" 
  mkdir -p "${ofp_install_dir}"
  cp "${build_path}"/ofp-sdf/fortran/pp/Fortran.pp      "${ofp_install_dir}"
  cp "${build_path}"/ofp-sdf/fortran/trans/fast2pp      "${ofp_install_dir}"
  cp "${build_path}"/ofp-sdf/fortran/trans/ofp2fast     "${ofp_install_dir}"
  cp "${build_path}"/ofp-sdf/fortran/syntax/Fortran.tbl "${ofp_install_dir}"
  pushd "${OPENCOARRAYS_SRC_DIR}"/3rd-party-tools/open-fortran-parser-sdf/trans/
  make OFP_HOME=${ofp_build_dir} fast-to-ocaf
  cp fast-to-caf "${ofp_install_dir}"
  popd
 #cp "${OPENCOARRAYS_SRC_DIR}"/3rd-party-tools/open-fortran-parser-sdf/trans/fast-to-ocaf.str "${ofp_install_dir}"
}

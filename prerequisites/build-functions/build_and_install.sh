# Make the build directory, configure, and build
# shellcheck disable=SC2154
build_and_install()
{
  num_threads=${arg_j}
  build_path="${OPENCOARRAYS_SRC_DIR}/prerequisites/builds/${package_to_build}-${version_to_build}"

  info "Building ${package_to_build} ${version_to_build}"
  info "Build path: ${build_path}"
  info "Installation path: ${install_path}"

  set_SUDO_if_needed_to_write_to_directory "${build_path}"
  set_SUDO_if_needed_to_write_to_directory "${install_path}"
  mkdir -p "${build_path}"
  info "pushd ${build_path}"
  pushd "${build_path}"
  if [[ "${package_to_build}" == "gcc" ]]; then
    info "pushd ${download_path}/${package_source_directory} "
    pushd "${download_path}/${package_source_directory}"
    "${PWD}"/contrib/download_prerequisites
    info "popd"
    popd
    info "Configuring gcc/g++/gfortran builds with the following command:"
    info "${download_path}/${package_source_directory}/configure --prefix=${install_path} --enable-languages=c,c++,fortran,lto --disable-multilib --disable-werror"
    "${download_path}/${package_source_directory}/configure" --prefix="${install_path}" --enable-languages=c,c++,fortran,lto --disable-multilib --disable-werror
    info "Building with the following command: 'make -j${num_threads} bootstrap'"
    make "-j${num_threads}" bootstrap
    if [[ ! -z "${SUDO:-}" ]]; then
      info "You do not have write permissions to the installation path ${install_path}"
      info "If you have administrative privileges, enter your password to install ${package_to_build}"
    fi
    info "Installing with the following command: ${SUDO:-} make install"
    ${SUDO:-} make install
  else
    info "Configuring ${package_to_build} ${version_to_build} with the following command:"
    info "FC=\"${FC:-'gfortran'}\" CC=\"${CC:-'gcc'}\" CXX=\"${CXX:-'g++'}\" \"${download_path}/${package_source_directory}\"/configure --prefix=\"${install_path}\""
    FC="${FC:-'gfortran'}" CC="${CC:-'gcc'}" CXX="${CXX:-'g++'}" "${download_path}/${package_source_directory}"/configure --prefix="${install_path}"
    info "Building with the following command:"
    info "FC=\"${FC:-'gfortran'}\" CC=\"${CC:-'gcc'}\" CXX=\"${CXX:-'g++'}\" make -j\"${num_threads}\""
    FC="${FC:-'gfortran'}" CC="${CC:-'gcc'}" CXX="${CXX:-'g++'}" make "-j${num_threads}"
    info "Installing ${package_to_build} in ${install_path}"
    if [[ ! -z "${SUDO:-}" ]]; then
      info "You do not have write permissions to the installation path ${install_path}"
      info "If you have administrative privileges, enter your password to install ${package_to_build}"
    fi
    info "Installing with the following command: ${SUDO:-} make install"
    ${SUDO:-} make install
  fi
  info "popd"
  popd
}

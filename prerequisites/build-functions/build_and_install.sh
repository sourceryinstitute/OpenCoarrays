# Make the build directory, configure, and build
# shellcheck disable=SC2154

source ${OPENCOARRAYS_SRC_DIR}/prerequisites/build-functions/edit_GCC_download_prereqs_file_if_necessary.sh

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

  if [[ "${package_to_build}" != "gcc" ]]; then

    if [[ "${package_to_build}" == "mpich" && "${version_to_build}" == "3.2"  && "${OSTYPE}" == "darwin"* ]]; then
      info "Patching MPICH 3.2 on Mac OS due to segfault bug."
      sed -i '' 's/} MPID_Request ATTRIBUTE((__aligned__(32)));/} ATTRIBUTE((__aligned__(32))) MPID_Request;/g' \
	  "${download_path}/${package_source_directory}/src/include/mpiimpl.h"
    fi

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

  else # ${package_to_build} == "gcc"

    # Use GCC's contrib/download_prerequisites script after modifying it, if necessary, to use the
    # the preferred download mechanism set in prerequisites/build-functions/set_or_print_downloader.sh

    info "pushd ${download_path}/${package_source_directory} "
    pushd "${download_path}/${package_source_directory}"

    # Switch download mechanism, if wget is not available
    edit_GCC_download_prereqs_file_if_necessary
   
    # Download GCC prerequisities
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

  fi # end if [[ "${package_to_build}" != "gcc" ]]; then
  
  info "popd"
  popd
}

#shellcheck shell=bash
# shellcheck disable=SC2154
build_opencoarrays()
{
  print_header
  info "Invoking find_or_install mpich"
  find_or_install mpich
  info "Invoking find_or_install cmake"
  find_or_install cmake
  build_path="${build_path}"/opencoarrays/$("${opencoarrays_src_dir}"/install.sh -V opencoarrays)
  mkdir -p "$build_path"
  pushd "$build_path"
  if [[ -z "${MPICC:-}" || -z "${MPIFC:-}" || -z "${CMAKE:-}" || -z "${FC:-}" || -z "${CC:-}" ]]; then
    emergency "Empty MPICC=$MPICC or MPIFC=$MPIFC or FC or CC or CMAKE=$CMAKE [exit 90]"
  else
    info "Configuring OpenCoarrays in ${PWD} with the command:"
    info "CC=\"${CC}\" FC=\"${FC}\" $CMAKE \"${opencoarrays_src_dir}\" -DCMAKE_INSTALL_PREFIX=\"${install_path}\" -DMPI_C_COMPILER=\"${MPICC}\" -DMPI_Fortran_COMPILER=\"${MPIFC}\""
    CC="${CC}" FC="${FC}" $CMAKE "${opencoarrays_src_dir}" -DCMAKE_INSTALL_PREFIX="${install_path}" -DMPI_C_COMPILER="${MPICC}" -DMPI_Fortran_COMPILER="${MPIFC}"
    info "Building OpenCoarrays in ${PWD} with the command make -j${num_threads}"
    make "-j${num_threads}"
    if [[ ! -z ${SUDO:-} ]]; then
      printf "\nThe chosen installation path requires sudo privileges. Please enter password if prompted.\n"
    fi
    info "Installing OpenCoarrays in ${install_path} with the command ${SUDO:-} make install"
    ${SUDO:-} make install
  fi
}

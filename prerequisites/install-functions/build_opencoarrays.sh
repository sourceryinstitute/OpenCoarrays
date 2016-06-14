# shellcheck disable=SC2154
build_opencoarrays()
{
  print_header
  info "Invoking find_or_install mpich"
  find_or_install mpich
  info "Invoking build_ofp_if_necesssary"
  build_ofp_if_necessary 
  info "Invoking find_or_install cmake"
  find_or_install cmake
  build_path="${build_path}"/opencoarrays/$("${opencoarrays_src_dir}"/install.sh -V opencoarrays)
  mkdir -p "$build_path"
  pushd "$build_path"
  if [[ -z "${MPICC:-}" || -z "${MPIFC:-}" || -z "${CMAKE:-}" ]]; then
    emergency "Empty MPICC=$MPICC or MPIFC=$MPIFC or CMAKE=$CMAKE [exit 90]"
  else
    info "Configuring OpenCoarrays in ${PWD} with the command:"
    info "CC=\"${MPICC}\" FC=\"${MPIFC}\" $CMAKE \"${opencoarrays_src_dir}\" -DCMAKE_INSTALL_PREFIX=\"${install_path}\""
    CC="${MPICC}" FC="${MPIFC}" $CMAKE "${opencoarrays_src_dir}" -DCMAKE_INSTALL_PREFIX="${install_path}"
    info "Building OpenCoarrays in ${PWD} with the command make -j${num_threads}"
    make "-j${num_threads}"
    if [[ ! -z ${SUDO:-} ]]; then
      printf "\nThe chosen installation path requires sudo privileges. Please enter password if prompted.\n"
    fi
    info "Installing OpenCoarrays in ${install_path} with the command ${SUDO:-} make install"
    ${SUDO:-} make install
    if [[ "${translate_source}" == "true" ]]; then
      install_ofp
    fi
  fi
}

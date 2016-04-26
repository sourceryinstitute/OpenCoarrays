build_opencoarrays()
{
  print_header
  info "find_or_install mpich"
  find_or_install mpich    
  info "find_or_install cmake"
  find_or_install cmake    
  mkdir -p $build_path     
  pushd $build_path           
  if [[ -z "$MPICC" || -z "$MPIFC" || -z "$CMAKE" ]]; then
    emergency "Empty MPICC=$MPICC or MPIFC=$MPIFC or CMAKE=$CMAKE [exit 90]"
  else
    info "Configuring OpenCoarrays in ${PWD} with the command:"
    info "CC=\"${MPICC}\" FC=\"${MPIFC}\" $CMAKE \"${opencoarrays_src_dir}\" -DCMAKE_INSTALL_PREFIX=\"${install_path}\""
    CC="${MPICC}" FC="${MPIFC}" $CMAKE "${opencoarrays_src_dir}" -DCMAKE_INSTALL_PREFIX="${install_path}" 
    info "Building OpenCoarrays in ${PWD} with the command make -j${num_threads}"
    make -j${num_threads}
    if [[ ! -z ${SUDO:-} ]]; then
      printf "\nThe chosen installation path requires sudo privileges. Please enter password if prompted.\n"
    fi 
    info "Installing OpenCoarrays in ${install_path} with the command ${SUDO:-} make install"
    ${SUDO:-} make install
  fi
}

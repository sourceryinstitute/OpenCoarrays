build_opencoarrays()
{
  print_header
  find_or_install mpich    
  find_or_install cmake    
  mkdir -p $build_path     
  cd $build_path           
  if [[ -z "$MPICC" || -z "$MPIFC" || -z "$CMAKE" ]]; then
    emergency "Empty MPICC=$MPICC or MPIFC=$MPIFC or CMAKE=$CMAKE [exit 90]"
  else
    CC="${MPICC}" FC="${MPIFC}" $CMAKE "${opencoarrays_src_dir}" -DCMAKE_INSTALL_PREFIX="${install_path}" 
    make -j$num_threads 
    if [[ ! -z ${SUDO:-} ]]; then
      printf "\nThe chosen installation path requires sudo privileges. Please enter password if prompted.\n"
    fi 
    ${SUDO:-} make install
  fi
}

work_around_cmake_issue()
{
  # This function ensures that the desired mpiexec is appears before any other mpiexec in the PATH of the process that runs
  # cmake in the build_opencoarrays function below.

  # This is a (hopefully temporary) workaround for CMake issue 16804 (https://gitlab.kitware.com/cmake/cmake/issues/16804):
  # even when MPI_C_COMPILER and MPI_Fortran_COMPILER are set as they are in the $CMAKE invocation below, cmake finds the
  # mpiexec program that is first in the PATH or fails to find any mpiexec if it's not in the PATH.

  MPIEXEC_PATH=${MPIFC%/*}
  if [[ ! -x ${MPIEXEC_PATH}/mpiexec ]]; then
    emergency "work_around_cmake_issue: executable mpiexec not found in ${MPIEXEC_PATH}"
  fi
  if [[ -z "${PATH}" ]]; then
    export PATH="${MPIEXEC_PATH}"
  else
    export PATH="${MPIEXEC_PATH}":"${PATH}"
  fi
}

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
  if [[ -z ${MPIFC:-} || -z ${MPICC:-} ]]; then
    emergency "build_opencoarrays.sh: empty \${MPIFC}=${MPIFC:-} or \${MPICC}=${MPICC:-}"
  fi
  MPIFC_show=($($MPIFC -show))
  MPICC_show=($($MPICC -show))
  if [[ "${MPIFC_show[0]}" != *gfortran* || "${MPICC_show[0]}" != *gcc* ]]; then
    emergency "build_opencoarrays.sh: MPI doesn't wrap gfortran/gcc: \${MPIFC_show}=${MPIFC_show[*]}, \${MPICC_show}=${MPICC_show[*]}"
  fi
  # Set FC to the MPI implementation's gfortran command with any preceding path but without any subsequent arguments:
  FC="${MPIFC_show[0]}"
  # Set CC to the MPI implementation's gcc command...
  CC="${MPICC_show[0]}"
  work_around_cmake_issue
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
}


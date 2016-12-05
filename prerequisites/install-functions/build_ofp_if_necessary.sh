# shellcheck disable=??????
function build_ofp_if_necessary()
{
  # arg_s (-s or --source-to-source) overrides any prior value of ${translate_true}
  if [[ "${arg_s}" == "${__flag_present}" ]]; then  

    translate_source="true"

  elif [[ "${translate_source:-}" != "false" && "${translate_source:-}" != "true" ]]; then
 
    # check the compiler identify and version only if a definitive answer has not yet been
    # reached (should only happen if -M or --mpi-path (arg_M) was specifid, in which case
    # the find_or_install.sh script defers the compiler check so that any such deferrals
    # all result in the check happening in one place (for now, there is only one place where
    # such a deferral happens).

    info "install_ofp_if_necessary: Checking whether ${MPIFC} wraps gfortran... "
    mpif90_version_header=$("${MPIFC}" --version | head -1)
    first_three_characters=$(echo "${mpif90_version_header}" | cut -c1-3)

    if [[ "${first_three_characters}" != "GNU" ]]; then
      info "no."
      translate_source="true" 
    else
      info "yes."
      info "install_ofp_if_necesssary: Checking whether ${MPIFC} wraps gfortran version $(./build.sh -V gcc) or later... "
      "${MPIFC}" acceptable_compiler.f90 -o acceptable_compiler
      "${MPIFC}" print_true.f90 -o print_true
      acceptable=$(./acceptable_compiler)
      is_true=$(./print_true)
      rm acceptable_compiler print_true

      if [[ "${acceptable}" == "${is_true}" ]]; then
         translate_source="false" 
      else
         translate_source="true" 
      fi
    fi
  fi
  if [[ "${translate_source}" == "true" ]]; then
      ofp_build_prefix="${OPENCOARRAYS_SRC_DIR}"/prerequisites/builds
      info "Building the Open Fortran Parser (ofp-sdf) with the following command:" 
      info "\"${OPENCOARRAYS_SRC_DIR}\"/prerequisites/build-ofp.sh \"--build-dir=${ofp_build_prefix}\""
      export ofp_build_dir="${ofp_build_prefix}/ofp-sdf"
      export arg_y=${arg_y:-}
      "${OPENCOARRAYS_SRC_DIR}"/prerequisites/build-ofp.sh "--build-dir=${ofp_build_prefix}"
  else
    info "No source translation required for the chosen compiler. Skipping Open Fortran Parser installation."
  fi
}

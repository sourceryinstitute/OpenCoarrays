# shellcheck shell=bash disable=SC2154,SC2148
print_header() {
  pushd "${__dir}" >/dev/null
  local gccver mpichver cmakever flexver bisonver m4ver
  gccver="$("${__file}" -V gcc)"
  mpichver="$("${__file}" -V mpich)"
  cmakever="$("${__file}" -V cmake)"
  flexver="$("${__file}" -V flex)"
  bisonver="$("${__file}" -V bison)"
  m4ver="$("${__file}" -V m4)"
  popd >/dev/null
  clear
  echo ""
  echo "*** By default, building OpenCoarrays requires CMake ${cmakever} or later,      ***"
  echo "*** MPICH ${mpichver}, and GCC Fortran (gfortran) ${gccver} or later.  To see         ***"
  echo "*** options for forcing the use of older or alternative packages, execute ***"
  echo "*** this script with the -h flag.  This script will recursively traverse  ***"
  echo "*** the following dependency tree, asking permission to download, build,  ***"
  echo "*** and install any packages that are required for building another       ***"
  echo "*** package and are neither in your PATH nor in                           ***"
  echo "*** opencoarrays/prerequisites/installations:                             ***"
  echo ""
  # Generate the following text using the `tree` command w/ dummy directory structure
  cat <<-EOF
    opencoarrays
    ├── cmake-${cmakever}
    └── mpich-${mpichver}
        └── gcc-${gccver}
            ├── flex-${flexver}
            │   └── bison-${bisonver}
            │       └── m4-${m4ver}
            ├── gmp
            ├── mpc
            └── mpfr

EOF
  echo ""
  printf "%s will be installed in %s\n" "${arg_p}" "${install_path}"
  echo ""
  if [[ "${arg_y}" == "${__flag_present}" ]]; then
    info "-y or --yes-to-all flag present. Proceeding with non-interactive build."
  else
    printf "Ready to rock and roll? (Y/n)"
    read -r install_now
    printf '%s\n' "${install_now}"
    if [[ "${install_now}" == "n" || "${install_now}" == "no" ]]; then
      emergency "${this_script}: Aborting. [exit 85]"
    fi
  fi
}

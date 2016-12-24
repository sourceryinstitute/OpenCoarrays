# Make the build directory, configure, and build
# shellcheck disable=SC2154
function version { echo "$@" | gawk -F. '{ printf("%03d%03d%03d\n", $1,$2,$3); }'; }

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

    backup_extension=".original"
    wget_line=`grep wget "${PWD}/contrib/download_prerequisites" | head -1` || true
    if [[ ! -z "${wget_line:-}"   ]]; then

      already_modified_downloader="false"

    else # Check whether gcc contrib/download_prerequisites was already backed up and then modified
      
      if [[ ! -f "${PWD}/contrib/download_prerequisites${backup_extension}" ]]; then
        emergency "build_and_install.sh: gcc contrib/download_prerequisites does not use wget"
      fi
      already_modified_downloader="true"
    fi

    if [[ ${already_modified_downloader:-} != "true"  ]]; then

      # Check for wget format used in GCC 5-6
      if [[ "${wget_line}" == *"ftp"* ]]; then
        wget_command="${wget_line%%ftp*}"

      # Check for wget format adopted in GCC 7
      elif [[  "${wget_line}" == *"base_url"* ]]; then 
        wget_command="${wget_line%%\"\$\{directory\}/\$\{ar\}\"*}"
        wget_command="wget${wget_command#*wget}"
      else
        emergency "build_and_install.sh: gcc contrib/download_prerequisites does not use a known URL format"
      fi
      info "GCC contrib/download_prerequisites wget command is '${wget_command}'"
  
      arg_string="${gcc_prereqs_fetch_args[@]:-}"
      gcc_download_changed=7.0.0
      if [[ "$(version "$version_to_build")" -gt "$(version "$gcc_download_changed")" || $version_to_build == "trunk" || $version_to_build == "$gcc_download_changed" ]]; then
        case "${gcc_prereqs_fetch}" in
          "curl")
            arg_string="${arg_string} -o "
          ;;
          "wget")
            debug "build_and_install.sh: using wget"  
          ;;
          *)
            debug "build_and_install.sh: if problems occur, check whether the modification of download_prerequisites is compatible with the download method ${gcc_prereqs_fetch}"  
          ;;
        esac
        if ! type sha512sum &> /dev/null; then
          info "build_and_install.sh: sha512sum unavailable. Turning off file integrity verification in GCC contrib/download_prerequisites."
          sed -i''${backup_extension} s/"verify=1"/"verify=0"/ "${PWD}/contrib/download_prerequisites"
        fi
      fi
      # Don't try to overwrite the unmodified download_prerequisites file if exists or if we don't have write permissions to it
      if [[ -f "${PWD}/contrib/download_prerequisites"${backup_extension} ]]; then
         backup_extension=""
      fi
      info "Replacing GCC contrib/download_prerequisites wget with the following command:"
      if [[ "$(uname)" == "Linux" ]]; then
        info "sed -i'${backup_extension}' s/\"${wget_command}\"/\"${gcc_prereqs_fetch} ${arg_string}\"/ \"${PWD}/contrib/download_prerequisites\""
        sed -i''${backup_extension} s/"${wget_command}"/"${gcc_prereqs_fetch} ${arg_string}"/ "${PWD}/contrib/download_prerequisites"
      else
        # This works on OS X and other POSIX-compliant operating systems:
        info "sed -i '${backup_extension}' s/\"${wget_command}\"/\"${gcc_prereqs_fetch} ${arg_string}\"/ \"${PWD}/contrib/download_prerequisites\""
        sed -i ''${backup_extension} s/"${wget_command}"/"${gcc_prereqs_fetch} ${arg_string}"/ "${PWD}/contrib/download_prerequisites"
      fi 

    fi # end if [[ ${already_modified_downloader:-} != "true"  ]];
    
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

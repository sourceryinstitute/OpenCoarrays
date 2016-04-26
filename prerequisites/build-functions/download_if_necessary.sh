source "${OPENCOARRAYS_SRC_DIR:-}/prerequisites/build-functions/ftp-url.sh"
source "${OPENCOARRAYS_SRC_DIR:-}/prerequisites/build-functions/set_SUDO_if_needed_to_write_to_directory.sh"

# Download pkg-config if the tar ball is not already in the present working directory
download_if_necessary()
{
  download_path="${PWD}/downloads"
  set_SUDO_if_needed_to_write_to_directory "${download_path}"
  if [ -f $url_tail ] || [ -d $url_tail ]; then
    info "Found '${url_tail}' in ${PWD}."
    info "If it resulted from an incomplete download, building ${package_name} could fail."
    info "Would you like to proceed anyway? (Y/n)"
    read proceed
    if [[ "${proceed}" == "n" || "${proceed}" == "N" || "${proceed}" == "no"  ]]; then
      info "n"
      info "Please remove $url_tail and restart the installation to to ensure a fresh download." 1>&2
      emergency "Aborting. [exit 80]"
    else
      info "y"
    fi
  elif ! type "${fetch}" &> /dev/null; then
    # The download mechanism is missing
    info "The default download mechanism for ${package_name} is ${fetch}."
    info "Please either ensure that ${fetch} is installed and in your PATH"
    info "or download the ${package_name} source from "
    info "${package_url}" 
   #called_by_install_sh=`echo "$(ps -p $PPID -o args=)" | grep install.sh`
    info "Place the downloaded file in ${download_path} and restart this script."
   #if [[ ! -z $called_by_install_sh ]]; then
   #  caller="install.sh"
   #else
   #  caller="build"
   #fi
    emergency "Aborting [exit 90]"
  else
    # The download mechanism is in the path.
    if [[ "${fetch}" == "svn" ]]; then
      if [[ ${version_to_build} == '--avail' || ${version_to_build} == '-a' ]]; then
        args="ls"
      else
        args="checkout"
      fi
    elif [[ "${fetch}" == "wget" ]]; then
      args="--no-check-certificate"
    elif [[ "${fetch}" == "ftp-url" ]]; then
      args="-n"
    elif [[ "${fetch}" == "git" ]]; then
      args="clone"
    fi
    if [[ "${fetch}" == "svn" || "${fetch}" == "git" ]]; then
      package_source_directory="${url_tail}"
    else
      package_source_directory="${package_name}-${version_to_build}"
    fi
    info "Downloading ${package_name} ${version_to_build} to the following location:"
    info "${download_path}/${package_source_directory}"
    info "Download command: ${fetch} ${args} ${package_url}"
    info "Depending on the file size and network bandwidth, this could take several minutes or longer."
    pushd "${download_path}"
    "${fetch}" "${args}" "${package_url}" 
    popd
    if [[ "${version_to_build}" == '--avail' || "${version_to_build}" == '-a' ]]; then
      # In this case, args="ls" and the list of available versions has been printed so we can move on.
      exit 1
    fi
    if [[ "${fetch}" == "svn" ]]; then
      search_path="${download_path}/${version_to_build}"
    else
      search_path="${download_path}/${url_tail}"
    fi
    if [ -f "${search_path}" ] || [ -d "${search_path}" ]; then
      info "Download succeeded. The "${package_name}" source is in the following location:"
      info "${search_path}"
    else
      info "Download failed. The "${package_name}" source is not in the following, expected location:"
      info "${search_path}"
      emergency "Aborting. [exit 110]"
    fi
  fi
}

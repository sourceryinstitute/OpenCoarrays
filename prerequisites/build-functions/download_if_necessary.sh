# shellcheck source=./ftp-url.sh
source "${OPENCOARRAYS_SRC_DIR}/prerequisites/build-functions/ftp-url.sh"
# shellcheck source=./set_SUDO_if_needed_to_write_to_directory.sh
source "${OPENCOARRAYS_SRC_DIR}/prerequisites/build-functions/set_SUDO_if_needed_to_write_to_directory.sh"

# Download package if the tar ball is not already in the present working directory
# shellcheck disable=SC2154
download_if_necessary()
{
  download_path="${OPENCOARRAYS_SRC_DIR}/prerequisites/downloads"
  set_SUDO_if_needed_to_write_to_directory "${download_path}"
  if  [[ -f "${download_path}/${url_tail}" || -d "${download_path}/${url_tail}" ]] ; then
    info "Found '${url_tail}' in ${download_path}."
    info "If it resulted from an incomplete download, building ${package_name} could fail."
    info "Would you like to proceed anyway? (Y/n)"
    read -r proceed
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
    info "Place the downloaded file in ${download_path} and restart this script."
    emergency "Aborting [exit 90]"
  else
    # The download mechanism is in the path.
    if [[ "${fetch}" == "svn" ]]; then
      if [[ "${arg_B:-}" == "gcc" ]]; then
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
    elif [[ "${fetch}" == "curl" ]]; then
      first_three_characters=$(echo "${package_url}" | cut -c1-3)
      if [[ "${first_three_characters}" == "ftp"  ]]; then 
        args="-LO -u anonymous:"
      elif [[ "${first_three_characters}" == "htt"  ]]; then 
        args="-LO"
      else
        emergency "download_if_necessary.sh: Unrecognized URL."
      fi
    fi

    if [[ "${fetch}" == "svn" || "${fetch}" == "git" ]]; then
      package_source_directory="${url_tail}"
    else
      package_source_directory="${package_name}-${version_to_build}"
    fi
    info "Downloading ${package_name} ${version_to_build-} to the following location:"
    info "${download_path}/${package_source_directory}"
    info "Download command: \"${fetch}\" ${args:-} ${package_url}"
    info "Depending on the file size and network bandwidth, this could take several minutes or longer."
    pushd "${download_path}"
    "${fetch}" ${args:-} ${package_url}
    popd
    if [[ ! -z "${arg_B:-}" ]]; then
      return
    else
      if [[ "${fetch}" == "svn" ]]; then
        search_path="${download_path}/${version_to_build}"
      else
        search_path="${download_path}/${url_tail}"
      fi
      if [[ -f "${search_path}" || -d "${search_path}" ]]; then
        info "Download succeeded. The ${package_name} source is in the following location:"
        info "${search_path}"
      else
        info "Download failed. The ${package_name} source is not in the following, expected location:"
        info "${search_path}"
        emergency "Aborting. [exit 110]"
      fi
    fi
  fi
}

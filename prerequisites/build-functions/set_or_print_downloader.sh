# shellcheck shell=bash disable=SC2148
# If -p, -P, -U, or -V specifies a package, set fetch variable
# If -D specifies a package, print "${fetch}" and exit with normal status
# If -l is present, list all packages and versions and exit with normal status
# shellcheck disable=SC2154
set_or_print_downloader()
{
  # Verify requirements
  [ ! -z "${arg_D}" ] && [ ! -z "${arg_p:-${arg_P:-${arg_U:-${arg_V}}}}" ] &&
    emergency "Please pass only one of {-D, -p, -P, -U, -V} or a longer equivalent (multiple detected)."

  package_name="${arg_p:-${arg_D:-${arg_P:-${arg_U:-${arg_V}}}}}"

  if [[ "${package_name}" == "gcc" ]]; then
    arg_b=${arg_b:-releases/gcc-${version_to_build}}
  elif [[ "${package_name}" == "ofp" ]]; then
    "${OPENCOARRAYS_SRC_DIR}/prerequisites/install-ofp.sh" "${@}"
    exit 0
  fi

  # Choose the first available download mechanism, prioritizing first any absolute requirement
  # (git for gcc) and second robustness:
  info "Checking available download mechanisms: ftp, wget, and curl."
  info "\${package_name}=${package_name}  \${arg_b:-}=${arg_b:-}"

  if type curl &> /dev/null; then
    gcc_prereqs_fetch=curl
  elif type wget &> /dev/null; then
    gcc_prereqs_fetch=wget
  elif type ftp &> /dev/null; then
    if [[ "${package_name}" == "wget" || "${package_name}" == "make" ||
          "${package_name}" == "bison" || "${package_name}" == "m4"   ]]; then
      gcc_prereqs_fetch=ftp_url
    fi
  else
    tried="curl, wget, and ftp"
  fi

  if [[ "${package_name}" == "gcc" ]]; then
    if type git &> /dev/null; then
      fetch=git
    else
      tried="git"
    fi
  else
    fetch=${gcc_prereqs_fetch}
  fi

  if [[ -z "${fetch:-}" ]]; then
    emergency "No available download mechanism. Option(s) tried: ${tried}"
  fi

  # If a printout of the download mechanism was requested, then print it and exit with normal status
  if [[ ! -z "${arg_D}" ]]; then
     printf "%s\n" "${fetch}"
     exit 0
  fi
}

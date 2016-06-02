# If -p, -P, -U, or -V specifies a package, set fetch variable
# If -D specifies a package, print "${fetch}" and exit with normal status
# If -l is present, list all packages and versions and exit with normal status
# shellcheck disable=SC2154
set_or_print_downloader()
{
  # Verify requirements
  [ ! -z "${arg_D}" ] && [ ! -z "${arg_p:-${arg_P:-${arg_U:-${arg_V:-${arg_B}}}}}" ] &&
    emergency "Please pass only one of {-B, -D, -p, -P, -U, -V} or a longer equivalent (multiple detected)."

  package_name="${arg_p:-${arg_D:-${arg_P:-${arg_U:-${arg_V:-${arg_B}}}}}}"

  if [[ "${package_name}" == "ofp" ]]; then
    ${OPENCOARRAYS_SRC_DIR}/prerequisites/install-ofp.sh "${@}"
    exit 0
  fi  

  # Choose the first available download mechanism, prioritizing first any absolute requirement 
  # (svn for gcc development branches) and second robustness:
  info "Checking available download mechanisms: ftp, wget, and curl."
  info "\${package_name}=${package_name}  \${arg_b:-\${arg_B:-}}=${arg_b:-${arg_B:-}}"
   
  if [[ "${package_name}" == "gcc" && ! -z "${arg_b:-${arg_B:-}}" ]]; then

    if type svn &> /dev/null; then
      fetch=svn 
    else 
      tried="svn"
    fi

  elif type curl &> /dev/null; then
    fetch=curl
  elif type wget &> /dev/null; then
    fetch=wget
  elif type ftp &> /dev/null; then
    if [[ "${package_name}" == "gcc"   || "${package_name}" == "wget" || "${package_name}" == "make" ||
          "${package_name}" == "bison" || "${package_name}" == "m4"   ]]; then
      fetch=ftp-url
    fi
  else
    tried="curl, wget, and ftp"
  fi  

  if [[ -z "${fetch:-}" ]]; then
    if [[ -z "${arg_B:-}" ]]; then
      warning "No available download mechanism. Options tried: ${tried}" 
    else 
      emergency "No available download mechanism. Option tried: ${tried}" 
    fi
  fi

  # If a printout of the download mechanism was requested, then print it and exit with normal status
  if [[ ! -z "${arg_D}" ]]; then 
     printf "%s\n" "${fetch}" 
     exit 0
  fi
}

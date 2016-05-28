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
  if [[ $(uname) == "Darwin" ]]; then
    wget_or_curl=curl
    ftp_or_curl=curl
  else
    wget_or_curl=wget
    ftp_or_curl=ftp-url
  fi
  if [[ "${package_name}" == "gcc" ]]; then
    if [[ -z "${arg_b:-${arg_B:-}}" ]]; then
      gcc_fetch="${ftp_or_curl}"
    else
      gcc_fetch="svn"
    fi
  fi
  # This is a bash 3 hack standing in for a bash 4 hash (bash 3 is the lowest common
  # denominator because, for licensing reasons, OS X only has bash 3 by default.)
  # See http://stackoverflow.com/questions/1494178/how-to-define-hash-tables-in-bash
  package_fetch=(
    "gcc:${gcc_fetch-}"
    "wget:${ftp_or_curl-}"
    "cmake:${wget_or_curl-}"
    "mpich:${wget_or_curl-}"
    "flex:${wget_or_curl-}"
    "bison:${ftp_or_curl-}"
    "pkg-config:${wget_or_curl-}"
    "make:${ftp_or_curl-}"
    "m4:${ftp_or_curl-}"
    "subversion:${wget_or_curl-}"
  )
  for package in "${package_fetch[@]}" ; do
     KEY="${package%%:*}"
     VALUE="${package##*:}"
     if [[ "${package_name}" == "${KEY}" ]]; then
       # We recognize the package name so we set the download mechanism:
       fetch=${VALUE}
       # If a printout of the download mechanism was requested, then print it and exit with normal status
       [[ ! -z "${arg_D}" ]] && printf "%s\n" "${fetch}" && exit 0
       break # exit the for loop
     fi
  done
  if [[ -z "${fetch:-}" ]]; then
    emergency "Package ${package_name:-} not recognized.  Use --l or --list-packages to list the allowable names."
  fi
}

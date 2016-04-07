# If -p, -P, -U, or -V specifies a package, set fetch variable
# If -D specifies a package, print "${fetch}" and exit with normal status
# If -l is present, list all packages and versions and exit with normal status
set_or_print_downloader()
{
  # Verify requirements 
  [ ! -z "${arg_D}" ] && [ ! -z "${arg_p:-${arg_P:-${arg_U:-${arg_V}}}}" ] &&
    emergency "Please pass only one of {-D, -p, -P, -U, -V} or a longer equivalent (multiple detected)."

  package_name="${arg_p:-${arg_D:-${arg_P:-${arg_U:-${arg_V}}}}}"
  if [[ "${package_name}" == "gcc" && "${version_to_build}" != "trunk" ]]; then
    gcc_fetch="ftp-url"
  else
    gcc_fetch="svn"
  fi  
  # This is a bash 3 hack standing in for a bash 4 hash (bash 3 is the lowest common
  # denominator because, for licensing reasons, OS X only has bash 3 by default.)
  # See http://stackoverflow.com/questions/1494178/how-to-define-hash-tables-in-bash
  package_fetch=(
    "gcc:$gcc_fetch"
    "wget:ftp-url"
    "cmake:wget"
    "mpich:wget"
    "flex:wget"
    "bison:ftp-url"
    "pkg-config:wget"
    "make:ftp-url"
    "m4:ftp-url"
    "subversion:wget"
  )
  for package in "${package_fetch[@]}" ; do
     KEY="${package%%:*}"
     VALUE="${package##*:}"
     if [[ "${package_name}" == "${KEY}" ]]; then
       # We recognize the package name so we set the download mechanism:
       fetch=${VALUE}
       # If a printout of the download mechanism was requested, then print it and exit with normal status 
       [[ ! -z "${arg_D}" ]] && printf "${fetch}\n" && exit 0
       break # exit the for loop
     fi
  done
  if [[ -z "${fetch:-}" ]]; then
    emergency "Package ${package_name:-} not recognized.  Use --l or --list-packages to list the allowable names."
  fi
}


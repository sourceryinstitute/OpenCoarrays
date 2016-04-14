# If -p, -D, -P, or -U specifies a package, set default_version
# If -V specifies a package, print the default_version and exit with normal status
# If -l is present, list all packages and versions and exit with normal status
set_or_print_default_version()
{
  # Verify requirements 
  [ "${arg_l}" == "${__flag_present}" ] && [ ! -z "${arg_D:-${arg_p:-${arg_P:-${arg_U:-${arg_V}}}}}" ] &&
    emergency "Please pass only one of {-l, -D, -p, -P, -U, -V} or a longer equivalent (multiple detected)."

  if [[ "${arg_l}" == "${__flag_present}" ]]; then 
    echo "This script can build the following packages:"
  fi
  # Get package name from argument passed with -p, -V, -D, or -U
  package_name="${arg_p:-${arg_D:-${arg_P:-${arg_U:-${arg_V}}}}}" # not needed for -l
  # This is a bash 3 hack standing in for a bash 4 hash (bash 3 is the lowest common
  # denominator because, for licensing reasons, OS X only has bash 3 by default.)
  # See http://stackoverflow.com/questions/1494178/how-to-define-hash-tables-in-bash
  package_version=(
    "cmake:3.4.0"
    "gcc:5.3.0"
    "mpich:3.1.4"
    "wget:1.16.3"
    "flex:2.6.0"
    "bison:3.0.4"
    "pkg-config:0.28"
    "make:4.1"
    "m4:1.4.17"
    "subversion:1.9.2"
  )
  for package in "${package_version[@]}" ; do
     KEY="${package%%:*}"
     VALUE="${package##*:}"
     if [[  "${arg_l}" == "${__flag_present}" ]]; then
       # If the list was requested, print the current element of the name:version list.
       printf "%s (default version %s)\n" "${KEY}" "${VALUE}"
     elif [[ "${package_name}" == "${KEY}" ]]; then
       # We recognize the package name so we set the default version:
       default_version=${VALUE}
       # If a printout of the default version number was requested, then print it and exit with normal status 
       [[ ! -z "${arg_V}" ]] && printf "${default_version}\n" && exit 0
       break # exit the for loop
     fi
  done

 # Exit with normal status (package/version has been printed).
 [ "${arg_l}" == "${__flag_present}" ] && exit 0

 # Exit with error status and diagnostic output if empty default_version
 if [[ -z "${default_version:-}" ]]; then
    emergency "Package ${package_name:-} not recognized.  Use --l or --list-packages to list the allowable names."
 fi
}

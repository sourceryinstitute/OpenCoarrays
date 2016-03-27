# Set and, if requested, print the default version(s) when either -l, -V, or -p are present on the command line
set_or_list_versions()
{
  # Verify requirements 
  if [[ "${arg_l}" == "${__flag_present}" ]]; then  
    if [[ ! -z "${arg_P}" || ! -z "${arg_U}" || ! -z "${arg_V}" || ! -z "${arg_p}" ]]; then
      emergency "Argument -l not allowed with -p, -P, -U, or -V (or the longer equivalents)."
    fi
    echo "This script can build the following packages:"
  elif [[ ! -z "${arg_V}" ]]; then
    [[ ! -z "${arg_P}" || ! -z "${arg_U}" || ! -z "${arg_p}" ]]  &&
      emergency "Argument -V not allowed with -p, -P, or -U (or the longer equivalents)."
  elif [[ ! -z "${arg_p}" ]]; then
    [[ ! -z "${arg_P}" || ! -z "${arg_U}" ]] && 
      emergency "Argument -p not allowed with -P or -U (or the longer equivalents)."
  else
    return # I got nothin' for ya.
  fi
  package_name="${arg_p:-${arg_V}}" # not needed for -l
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
    "_unknown:0"
  )
  for package in "${package_version[@]}" ; do
     KEY="${package%%:*}"
     VALUE="${package##*:}"
     if [[  "${arg_l}" == "${__flag_present}" ]]; then
       # If the list wass requested and we are not at the list terminator, print the current element of the name:version list.
       [[ "$KEY" !=  "_unknown" ]] && printf "%s (default version %s)\n" "${KEY}" "${VALUE}"
       list_printed=true
     elif [[ "${package_name}" == "${KEY}" ]]; then
       # We recognize the package name so we set the default version:
       default_version=${VALUE}
       [[ ! -z "${arg_V}" ]] && printf "${default_version}"
       break # exit the for loop
     fi
  done
 if [[ -z "${list_printed:-${default_version:-}}" ]]; then
    emergency "Package ${package_name:-} not recognized.  Use --l or --list-packages to list the allowable names."
  fi
}

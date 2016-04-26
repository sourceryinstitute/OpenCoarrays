# If -P specifies a package, print the installation path and exit with normal status
# Otherwise, set install_path
set_or_print_installation_path()
{
  # Verify requirements 
  [ ! -z "${arg_P}" ] && [ ! -z "${arg_D:-${arg_p:-${arg_U:-${arg_V}}}}" ] &&
    emergency "Please pass only one of {-D, -p, -P, -U, -V} or a longer equivalent (multiple detected)."
 
  # ../build.sh-usage specifies the following installation path default for illustrative purposes only:
  # Default="${OPENCOARRAYS_SRC_DIR}/prerequisites/installations/${package_name:-}/"
  # When the parse_command_line function executes, ${package_name:-} is empty, but it has 
  # been set before reaching the current function so now we can get the true values if the
  # user didn't override the default: 

  path_in_usage_file="${OPENCOARRAYS_SRC_DIR}/prerequisites/installations//"
  if [[ "${arg_i}" == "${path_in_usage_file}" ]]; then
    install_path="${OPENCOARRAYS_SRC_DIR}/prerequisites/installations/${package_name}/"
  else
    install_path="${arg_i}"
  fi

  # If -P is present, print ${install_path} and exit with normal status
  if [[ ! -z "${arg_P:-}" ]]; then 
    printf "${install_path}\n" 
    exit 0
  fi

  info "${package_name} ${version_to_build} installation path: ${install_path}" 
}

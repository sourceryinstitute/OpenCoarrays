# Define the sudo command to be used if the installation path requires administrative permissions
set_SUDO_if_necessary()
{
  if [[ -z "${LD_LIBRARY_PATH:-}" ]]; then
    SUDO_COMMAND="sudo"
  else
    SUDO_COMMAND="sudo env LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
  fi
  # shellcheck disable=SC2154
  printf "%s: %s installation path is %s" "$this_script" "$package_to_build" "${install_path}"
  printf "Checking whether the %s installation path exists... " "$package_to_build"
  if [[ -d "${install_path}" ]]; then
    printf "yes\n"
    printf "Checking whether I have write permissions to the installation path... "
    if [[ -w "${install_path}" ]]; then
      printf "yes\n"
    else
      printf "no\n"
      SUDO=$SUDO_COMMAND
    fi
  else
    printf "no\n"
    printf "Checking whether I can create the installation path... "
    if mkdir -p "${install_path}" >& /dev/null; then
      printf "yes.\n"
    else
      printf "no.\n"
      export SUDO=$SUDO_COMMAND
    fi
  fi
}

# Define the sudo command to be used if the installation path requires administrative permissions
set_SUDO_if_necessary()
{
  SUDO_COMMAND="sudo env LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
  printf "$this_script: $package_to_build installation path is $install_path"
  printf "Checking whether the $package_to_build installation path exists... "
  if [[ -d "$install_path" ]]; then
    printf "yes\n"
    printf "Checking whether I have write permissions to the installation path... "
    if [[ -w "$install_path" ]]; then
      printf "yes\n"
    else
      printf "no\n"
      SUDO=$SUDO_COMMAND
    fi
  else
    printf "no\n"
    printf "Checking whether I can create the installation path... "
    if mkdir -p $install_path >& /dev/null; then
      printf "yes.\n"
    else
      printf "no.\n"
      SUDO=$SUDO_COMMAND
    fi
  fi
}

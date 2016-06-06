# shellcheck shell=bash disable=SC2154,SC2148
print_header()
{
  clear
  echo ""
  echo "*** By default, building OpenCoarrays requires CMake 3.4.0 or later,      ***"
  echo "*** MPICH 3.1.4, and GCC Fortran (gfortran) 6.1.0 or later.  To see       ***"
  echo "*** options for forcing the use of older or alternative packages, execute ***"
  echo "*** this script with the -h flag.  This script will recursively traverse  ***"
  echo "*** the following dependency tree, asking permission to download, build,  ***"
  echo "*** and install any packages that are required for building another       ***"
  echo "*** package and are neither in your PATH nor in                           ***"
  echo "*** opencoarrays/prerequisites/installations:                             ***"
  echo ""
  # Move to a directory tree whose structure mirrors the dependency tree
  pushd "$opencoarrays_src_dir/prerequisites/dependency_tree/" > /dev/null
  if type tree &> /dev/null; then
    # dynamically compute and print the tree, suppressing the final line
    tree opencoarrays  | sed '$d'
  else
    # print the most recently saved output of the above 'tree' command
    sed '$d' < opencoarrays-tree.txt
  fi
  popd > /dev/null
  echo ""
  printf "%s will be installed in %s\n" "${arg_p}" "${install_path}"
  echo ""
  if [[ "${arg_y}" == "${__flag_present}" ]]; then
    info "-y or --yes-to-all flag present. Proceeding with non-interactive build."
  else
    printf "Ready to rock and roll? (Y/n)"
    read -r install_now
    echo -e " $install_now\n"
    if [[ "$install_now" == "n" || "$install_now" == "no" ]]; then
      emergency "$this_script: Aborting. [exit 85]\n"
    fi
  fi
}

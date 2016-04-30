# shellcheck disable=SC2154
print_header()
{
  clear
  echo ""
  echo "*** A default build of OpenCoarrays requires CMake 3.4.0 or later     ***"
  echo "*** and MPICH 3.1.4 wrapping GCC Fortran (gfortran) 5.3.0 or later.   ***"
  echo "*** Additionally, CMake, MPICH, and GCC have their own prerequisites. ***"
  echo "*** This script will check for most known requirements in your PATH   ***"
  echo "*** environment variable and in the default installation directory    ***"
  echo "*** to which this script installs each missing prerequisite.  The     ***"
  echo "*** script will recursively traverse the following dependency tree    ***"
  echo "*** and ask permission to download, build, and install any missing    ***"
  echo "*** prerequisites:                                                    ***"
  echo ""
  # Move to a directory tree whose structure mirrors the dependency tree
  pushd "$opencoarrays_src_dir/doc/dependency_tree/" > /dev/null
  if type tree &> /dev/null; then
    # dynamically compute and print the tree, suppressing the final line
    tree opencoarrays  | sed '$d'
  else
    # print the most recently saved output of the above 'tree' command
    sed '$d' < opencoarrays-tree.txt
  fi
  popd > /dev/null
  echo ""
  printf "*** OpenCoarrays and all prerequisites will be downloaded, built, and installed in ***\n"
  echo -e "${build_path}\n"
  echo -e "${install_path}\n"
  printf "Ready to rock and roll? (Y/n)"
  read -r install_now
  echo -e " $install_now\n"
  if [[ "$install_now" == "n" || "$install_now" == "no" ]]; then
    emergency "$this_script: Aborting. [exit 85]\n"
  fi
}

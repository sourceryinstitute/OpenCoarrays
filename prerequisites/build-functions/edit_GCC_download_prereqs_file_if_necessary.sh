# Make the build directory, configure, and build
# shellcheck disable=SC2154

edit_GCC_download_prereqs_file_if_necessary()
{
  # Only modify download_prerequisites if wget is unavailable
   if type wget &> /dev/null; then
     info "wget available. Invoking unmodified GCC script contrib/download_prerequisites."
   else 
    info "wget unavailable. Editing GCC contrib/download_prerequisites to replace it with ${gcc_prereqs_fetch}"

    download_prereqs_file="${PWD}/contrib/download_prerequisites"
  
    # Define a file extension for the download_prerequisites backup
    backup_extension=".original"
    backup_file="${download_prereqs_file}${backup_extension}"
    if [[ -f ${backup_file}  ]]; then
      # Prevent overwriting an existing backup:
      backup_extension=""
    fi
  
    # Grab the line with the first occurence of 'wget'
    wget_line=`grep wget "${download_prereqs_file}" | head -1` || true
    if [[ ! -z "${wget_line:-}"  ]]; then
      # Download_prerequisites contains wget so we haven't modified it
      already_modified_downloader="false"
    else 
      # Check whether a backup file already exists
      if [[ ! -f "${backup_file}" ]]; then
        emergency ": gcc contrib/download_prerequisites does not use wget"
      else
        already_modified_downloader="true"
      fi
    fi
  
    # Only modify download_prerequisites once
    if [[ ${already_modified_downloader} != "true"  ]]; then

      # Check for wget format used before GCC 7
      if [[ "${wget_line}" == *"ftp"* ]]; then
        gcc7_format="false"
        wget_command="${wget_line%%ftp*}" # grab everything before ftp

      # Check for wget format adopted in GCC 7
      elif [[  "${wget_line}" == *"base_url"* ]]; then 
        gcc7_format="true"
        case "${gcc_prereqs_fetch}" in
          "ftp_url")
            # Insert a new line after line 2 to include ftp_url.sh as a download option
            sed -i${backup_extension} -e '2 a\'$'\n'". ${OPENCOARRAYS_SRC_DIR}/prerequisites/build-functions/ftp_url.sh"$'\n' "${download_prereqs_file}"
            wget_command='wget --no-verbose -O "${directory}\/${ar}"'
          ;;
          "curl")
            wget_command="${wget_line%%\"\$\{directory\}*}" # grab everything before "${base_url}
            wget_command="wget${wget_command#*wget}" # keep everything from wget forward
          ;;
          *)
            emergency "Unknown download program ${gcc_prereqs_fetch} in edit_GCC_download_prereqs_file_if_necessary.sh"
          ;;
        esac

      else
        emergency "gcc contrib/download_prerequisites does not use a known URL format"
      fi

      arg_string="${gcc_prereqs_fetch_args[@]:-} "

      if [[ ${gcc7_format} == "true" ]]; then
        case "${gcc_prereqs_fetch}" in
          "curl")
            arg_string="${arg_string} -o "
          ;;
          *)
            debug "if problem downloading, ensure that the gcc download_prerequisites edits are compatible with ${gcc_prereqs_fetch}"  
          ;;
        esac
        # Protect against missing sha512sum command adopted in GCC 7 (not available by default on Lubuntu Linux)
        if ! type sha512sum &> /dev/null; then
          info "sha512sum unavailable. Turning off file integrity verification in GCC contrib/download_prerequisites."
          if [[ "$(uname)" == "Linux" ]]; then
            # Adjust for non-POSIX OS
            sed -i"${backup_extension}" s/"verify=1"/"verify=0"/ "${download_prereqs_file}"
          else
            sed -i "${backup_extension}" s/"verify=1"/"verify=0"/ "${download_prereqs_file}"
          fi
        fi
      fi
      info "Using the following command to replacing wget in the GCC download_prerequisites file:"
      info "sed -i${backup_extension} s/\"${wget_command}\"/\"${gcc_prereqs_fetch} ${arg_string} \"/ \"${download_prereqs_file}\""
      if [[ "$(uname)" == "Linux" ]]; then
        sed -i"${backup_extension}" s/"${wget_command}"/"${gcc_prereqs_fetch} ${arg_string} "/ "${download_prereqs_file}"
      else
        sed -i "${backup_extension}" s/"${wget_command}"/"${gcc_prereqs_fetch} ${arg_string} "/ "${download_prereqs_file}"
      fi

    fi # end if [[ ${already_modified_downloader:-} != "true"  ]];
   fi # end if ! type wget &> /dev/null; 
}

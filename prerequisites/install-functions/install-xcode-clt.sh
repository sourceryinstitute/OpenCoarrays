# shellcheck shell=bash disable=SC2154,SC2148
need_xcodeCLT() {
  if [[ "${OSTYPE}" != [Dd][Aa][Rr][Ww][Ii][Nn]* ]]; then
    echo "false"
  else
    local xcode_dir
    xcode_dir="$(/usr/bin/xcode-select -print-path 2>/dev/null)"
    if [[ ! -d "${xcode_dir}" || ! -x "${xcode_dir}/usr/bin/make" ]]; then
      echo "true"
    else
      echo "false"
    fi
  fi
}

xcode_clt_install () {
  info "It appears that you are on Mac OS and do not have the Xcode command line tools (CLT) installed."
  app_store_label="$(softwareupdate -l | grep -B 1 -E "Command Line (Developer|Tools)" | awk -F"*" '/^ +\\*/ {print $2}' | sed 's/^ *//' | tail -n1)"
  if [[ "${arg_y}" == "${__flag_present}" ]]; then
    info "-y or --yes-to-all flag present. Proceeding with non-interactive download and install."
  else
    printf "Ready to install %s? (Y/n)" "${app_store_label}"
    read -r install_xcodeclt
    printf '%s\n' "${install_xcodeclt}"
    if [[ "${install_xcodeclt}" == [nN]* ]]; then
      emergency "${this_script}: Aborting: cannot procede without XCode CLT."
    fi
  fi
  info "We will attempt to download and install XCode CLT for you now. Note: sudo priveledges required"
  place_holder="/tmp/.com.apple.dt.CommandLineTools.installondemand.in-progress"
  sudo /usr/bin/touch "${place_holder}" || true
  sudo /usr/sbin/softwareupdate -i app_store_label || true
  sudo /bin/rm -f "${place_holder}" || true
  sudo /usr/bin/xcode-select --switch "/Library/Developer/CommandLineTools" || true
}

headless_xcode_clt_install () {
  # Fallback her if headless install failed
  if [[ "${arg_y}" == "${__flag_present}" ]]; then
    info "-y or --yes-to-all flag present. Proceeding with non-interactive download and install."
  else
    printf "Ready to install Xcode-CLT using xcode-select? (Y/n)"
    read -r install_xcodeclt
    printf '%s\n' "${install_xcodeclt}"
    if [[ "${install_xcodeclt}" == [nN]* ]]; then
      emergency "${this_script}: Aborting: cannot procede without XCode CLT."
    fi
  fi
  info "Installing Xcode Command Line Tools (CLE), please follow instructions on popup window."
  sudo /usr/bin/xcode-select --install || emergency "${this_script}: unable to run \`sudo xcode-select --install\`"
  printf "Press <enter> once installation has completed"
  read -r
  sudo /usr/bin/xcode-select --switch "/Library/Developer/CommandLineTools" || \
    emergency "${this_script}: Xcode-CLT installation and activation failed, unable to continue"
}

maybe_install_xcodeCLT () {
  if [[ "$(need_xcodeCLT)" == "true" ]]; then
    xcode_clt_install
  fi
  if [[ "$(need_xcodeCLT)" == "true" ]]; then
    info "First Xcode-CLT installation failed, trying another method"
    headless_xcode_clt_install
  fi

  clang_output="$(/usr/bin/xzrun clang 2>&1)" || true
  if [[ "${clang_output}" =~ license ]]; then
    emergency "${this_script}: It appears you have not agreed to the Xcode license. Please do so before attempting to run this script again. This may be acheived by opening Xcode.app or running \`sudo xcodebuild -license\`"
  fi
}

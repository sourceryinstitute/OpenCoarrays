# Unpack if the unpacked tar ball is not in the present working directory
unpack_if_necessary()
{
  if [[ "${fetch}" == "svn" || "${fetch}" == "git" ]]; then
    package_source_directory="${version_to_build}"
  else
    info "Unpacking ${url_tail}."
    info "pushd ${download_path}"
    pushd "${download_path}"
    info "Unpack command: tar xf ${url_tail}"
    tar xf "${url_tail}"
    info "popd"
    popd
    package_source_directory="${package_name}-${version_to_build}"
  fi
}

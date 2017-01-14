# Brewfile to install OpenCoarrays prerequisites with [Homebrew] or [Linuxbrew].
#
# TL;DR: `brew tap homebrew/bundle && brew bundle` to install prerequisites
#
# Homebrew for Mac OS X can be installed via:
# `/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"`
# Linux brew (lives in userland, no sudo required) can be installed with:
# `ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Linuxbrew/linuxbrew/go/install)"`
#
# Once Homebrew/Linuxbrew is installed, execute the following commands to install the required prerequisites:
# `brew tap homebrew/bundle && brew bundle`
#
# NOTE: Until Linuxbrew upgrades to GCC 6.1 or later, you will have to edit the `mpich` line to:
#    `brew 'mpich', args: ['cc=gcc-5', 'cxx=g++-5']`

brew 'cmake'
brew 'gcc'
brew 'mpich', args: ['cc=gcc-6', 'cxx=g++-6']

# [Homebrew]: http://brew.sh
# [Linuxbrew]: http://linuxbrew.sh

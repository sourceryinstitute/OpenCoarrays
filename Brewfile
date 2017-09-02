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

brew 'cmake'
brew 'gcc'
brew 'mpich', args: ['cc=gcc-7', 'build-from-source']
brew 'opencoarrays', args: ['cc=gcc-7', 'build-from-source']

# [Homebrew]: http://brew.sh
# [Linuxbrew]: http://linuxbrew.sh

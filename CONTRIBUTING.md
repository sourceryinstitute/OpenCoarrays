<a name="top"> </a>

Contributing to OpenCoarrays
============================

[![](https://img.shields.io/github/issues-closed-raw/sourceryinstitute/OpenCoarrays.svg?style=flat-square)](https://github.com/sourceryinstitute/OpenCoarrays)
[![Issue Stats](https://img.shields.io/issuestats/i/github/sourceryinstitute/OpenCoarrays.svg?style=flat-square)](https://github.com/sourceryinstitute/OpenCoarrays/issues)
[![](https://img.shields.io/github/issues-pr-closed-raw/sourceryinstitute/OpenCoarrays.svg?style=flat-square)](https://github.com/sourceryinstitute/OpenCoarrays/pulls)
[![Issue Stats](https://img.shields.io/issuestats/p/github/sourceryinstitute/OpenCoarrays.svg?style=flat-square)](https://github.com/sourceryinstitute/OpenCoarrays/pulls)
[![Download as PDF][pdf img]](http://md2pdf.herokuapp.com/sourceryinstitute/OpenCoarrays/blob/master/CONTRIBUTING.pdf)

Download this file as a PDF document
[here](http://md2pdf.herokuapp.com/sourceryinstitute/OpenCoarrays/blob/master/CONTRIBUTING.pdf).

- [Reporting Defects](#reporting-defects)
- [Requesting Enhancements](#requesting-enhancements)
- [Helping Out](#helping-out)
  - [Outside Contributors](#outside-contributors)
- [OpenCoarrays Branches](#opencoarrays-branches)
  - [Master](#master)
  - [Devel](#devel)

Reporting Defects
-----------------

If you encounter problems during the course of [Installing] OpenCoarrays or [using OpenCoarrays], please take the following actions:

 1. Search the [issues] page (including [closed issues]) to see if anyone has encountered the same problem. If so add your experience to that thread.
 2. Search the [mailing list] to see if the issue has been discussed there.
 3. If unable to resolve the problem, please file a [new issue] and be sure to include the following information:
   - Fortran and companion C compiler, including version number, being used with OpenCoarrays
   - Communication library being used e.g., OpenMPI, MVAPICH or GASNet and the version number
   - Open Coarrays version number, or if building from `master`, commit SHA (`caf --version`)
   - Conditions required to reproduce the problem:
     - OS
     - Type of machine/hardware the code is running on
     - Number of MPI ranks/processing elements/coarray images being run on
     - How the code was compiled, including all flags and commands
     - Minimal reproducer code (a few lines) required to trigger the bug
 4. Any help you can provide diagnosing, isolating and fixing the
    problem is appreciated! Please see the [helping out] section for
    more information.

An [issue template] is in the `.github` folder to help ensure
compliance, and adequate information is provided.

Requesting Enhancements
-----------------------

If you would like OpenCoarrays to support a new communication library, OS, or have any other suggestions for its improvement, please take the following action:

 1. Search the [issues] page and [mailing list] to see if the feature or enhancement has already been requested.
 2. If not, please file a [new issue], and clearly explain your proposed enhancement.
 3. If you are able to help out in the implementation or testing of the proposed feature, please see the [helping out] section of this document.

Helping Out
-----------

Thank you for your interest in contributing to OpenCoarrays! Your help is very appreciated! Below are some tips and guidelines to get started.

### Outside Contributors ###

Here is a checklist to help you get started contributing to OpenCoarrays and walk you through the process:

 - [ ] Take a look at the [issues] page. Make sure that you're not about to duplicate someone else's work.
 - [ ] Post a [new issue] discussing the changes you're proposing to implement; whether bug fix(es) or enhancement(s)/feature request(s)--or give us a heads up that you are going to start work on [an open issue].
 - [ ] Please [Fork] the [OpenCoarrays repo] to your private repository.
 - [ ] Next [Create a branch] and make sure to include the issue number(s) in the branch name, for example: `issue-53-fix-install-dir-logic` or `fix-typo-#23`
 - [ ] Configure your local repository with the whitespace settings (and git hooks to enforce these) by running `./developer-scripts/setup-git.sh`. (Add the `--global` flag to this script to use these settings across all your new repositories, or newly cloned repositories.)  Pull requests introducing errant spaces and non-printing characters will not be accepted until these problems are addressed.
 - [ ] Make your changes and commit them to your local repository, following these guidelines:
   - [ ] Each commit should be a logically atomic, self-consistent, cohesive set of changes.
   - [ ] The code should compile and pass all tests after each commit.
   - [ ] The [commit message] should follow [these guidelines]:
     - [ ] First line is directive phrase, starting with a capitalized imperative verb, and is no longer than 50 characters summarizing your commit
     - [ ] Next line, if necessary is blank
     - [ ] Following lines are all wrapped at 72 characters and can include additional paragraphs, bulleted lists, etc.
     - [ ] Use [Github keywords] where appropriate, to indicate the commit resolves an open issue.
 - [ ] Please do your best to keep a [clean and coherent history]. `git add -p ...`, `git commit --amend` and `git rebase --interactive <root-ref>` can be helpful to rework your commits into a cleaner, clearer state.
 - [ ] Next, [open up a pull request] where the base branch is [`master`] or [`devel`] as appropriate
 - [ ] Please be patient and responsive to requests and comments from SourceryInstitute (SI) team members. You may be asked to amend or otherwise alter commits, or push new commits to your branch.
 - [ ] Make sure that all the automated [Travis-CI tests] pass
 - [ ] Sign the [Contributor License Agreement (CLA)] by clicking the "details" link to the right of the `licence/cla` check and following the directions on the CLA assistant webpage

OpenCoarrays Branches
---------------------

OpenCoarrays uses the [Github flow] workflow. There are [a number of resources] available to learn about the [Github flow] workflow, including a [video]. The gist of it is that the `master` branch is always deployable and deployed. The means at anytime, a new tagged release could be shipped using the `master` branch. For major changes that introduce experimental, or disruptive changes, we have a semi-stable `devel` branch.

### Master ###

The `master` branch should remain in pristine, stable condition all of the time. Any changes are applied atomically via pull requests. It should be assumed that customers are using the code on this branch, and great care should be taken to ensure its stability. Most bug fixes and incremental improvements will get merged into the `master` branch first, and then also the `devel` branch.


### Devel ###

This is the development branch, akin to GCC's `trunk`. Both of `devel` and `master` branches are protected, but `devel` will eventually be merged into `master` when the next major release happens, but until then it is a stable, forward looking branch where experimental features and major changes or enhancements may be applied and tested. Just as with `master` all changes are applied atomically as pull requests.

## Through put ##

[![Waffle.io](https://img.shields.io/waffle/label/sourceryinstitute/OpenCoarrays/blocked.svg?style=flat-square)](https://github.com/sourceryinstitute/OpenCoarrays/labels/blocked)
[![Waffle.io](https://img.shields.io/waffle/label/sourceryinstitute/OpenCoarrays/ready.svg?style=flat-square)](https://github.com/sourceryinstitute/OpenCoarrays/labels/ready)
[![Waffle.io](https://img.shields.io/waffle/label/sourceryinstitute/OpenCoarrays/in-progress.svg?style=flat-square)](https://github.com/sourceryinstitute/OpenCoarrays/labels/in-progress)
[![Waffle.io](https://img.shields.io/waffle/label/sourceryinstitute/OpenCoarrays/needs%20review.svg?style=flat-square)](https://github.com/sourceryinstitute/OpenCoarrays/labels/needs-review)

[![Throughput Graph](https://graphs.waffle.io/sourceryinstitute/OpenCoarrays/throughput.svg)](https://waffle.io/sourceryinstitute/OpenCoarrays/metrics/throughput)

## Coverage ##

[![Codecov branch](https://img.shields.io/codecov/c/github/sourceryinstitute/OpenCoarrays/master.svg?style=flat-square)](https://codecov.io/gh/sourceryinstitute/OpenCoarrays)

[![coverage history](https://codecov.io/gh/sourceryinstitute/OpenCoarrays/branch/master/graphs/commits.svg)](https://codecov.io/gh/sourceryinstitute/OpenCoarrays)

---

[![GitHub forks](https://img.shields.io/github/forks/sourceryinstitute/OpenCoarrays.svg?style=social&label=Fork)](https://github.com/sourceryinstitute/OpenCoarrays/fork)
[![GitHub stars](https://img.shields.io/github/stars/sourceryinstitute/OpenCoarrays.svg?style=social&label=Star)](https://github.com/sourceryinstitute/OpenCoarrays)
[![GitHub watchers](https://img.shields.io/github/watchers/sourceryinstitute/OpenCoarrays.svg?style=social&label=Watch)](https://github.com/sourceryinstitute/OpenCoarrays)
[![Twitter URL](https://img.shields.io/twitter/url/http/shields.io.svg?style=social)](https://twitter.com/intent/tweet?hashtags=HPC,Fortran,PGAS&related=zbeekman,gnutools,HPCwire,HPC_Guru,hpcprogrammer,SciNetHPC,DegenerateConic,jeffdotscience,travisci&text=Stop%20programming%20w%2F%20the%20%23MPI%20docs%20in%20your%20lap%2C%20try%20Coarray%20Fortran%20w%2F%20OpenCoarrays%20%26%20GFortran!&url=https%3A//github.com/sourceryinstitute/OpenCoarrays)


[Links]: #
[issue template]: https://github.com/sourceryinstitute/OpenCoarrays/blob/master/.github/ISSUE_TEMPLATE.md
[video]: https://youtu.be/EwWZbyjDs9c?list=PLg7s6cbtAD17uAwaZwiykDci_q3te3CTY
[a number of resources]: http://scottchacon.com/2011/08/31/github-flow.html
[Github flow]: https://guides.github.com/introduction/flow/
[Travis-CI tests]: https://travis-ci.org/sourceryinstitute/OpenCoarrays/pull_requests
[Contributor License Agreement (CLA)]: https://cla-assistant.io/sourceryinstitute/OpenCoarrays
[`master`]: https://github.com/sourceryinstitute/OpenCoarrays
[`devel`]: https://github.com/sourceryinstitute/OpenCoarrays/tree/devel
[open up a pull request]: https://github.com/sourceryinstitute/OpenCoarrays/compare
[clean and coherent history]: https://www.reviewboard.org/docs/codebase/dev/git/clean-commits/
[Github keywords]: https://help.github.com/articles/closing-issues-via-commit-messages/#closing-an-issue-in-a-different-repository
[commit message]: https://robots.thoughtbot.com/5-useful-tips-for-a-better-commit-message
[these guidelines]: http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html
[an open issue]: https://github.com/sourceryinstitute/OpenCoarrays/issues
[Create a branch]: https://help.github.com/articles/creating-and-deleting-branches-within-your-repository/
[OpenCoarrays repo]: https://github.com/sourceryinstitute/OpenCoarrays/fork
[Pull Request]: https://help.github.com/articles/using-pull-requests/
[Fork]: https://help.github.com/articles/fork-a-repo/
[helping out]: #helping-out
[closed issues]: https://github.com/sourceryinstitute/OpenCoarrays/issues?q=is%3Aissue+is%3Aclosed
[Installing]: ./INSTALLING.md
[issues]: https://github.com/sourceryinstitute/OpenCoarrays/issues
[mailing list]: https://groups.google.com/forum/#!forum/opencoarrays
[using OpenCoarrays]: ./GETTING_STARTED.md
[new issue]: https://github.com/sourceryinstitute/OpenCoarrays/issues/new
[pdf img]: https://img.shields.io/badge/PDF-CONTRIBUTING.md-6C2DC7.svg?style=flat-square "Download as PDF"

<!-- Please fill out the pull request template included below, failure -->
<!-- to do so may result in immediate closure of your pull request. -->

<!-- Fill out all portions of this template that apply. Please delete -->
<!-- any unnecessary sections. -->

| Avg response time                 | coverage on master         |
| --------------------------------- | ---------------------------|
| ![Issue Stats][PR response img]   | ![Codecov branch][coverage]|

This pull request (PR) is a:
 - [ ] Bug fix
 - [ ] Feature addition
 - [ ] Other, Please describe:

I certify that:

 - [ ] I have reviewed the [contributing guidelines]
 - [ ] If this PR is a work in progress I have added `WIP:` to the
       beginning of the PR title
 - [ ] If this PR is problematic for any reason, I have added
       `DO NOT MERGE:` to the beginning of the title
 - [ ] The branch name and title of this PR contains the text
       `issue-<#>` where `<#>` is replaced by the issue that this PR
       is addressing
 - [ ] I have deleted trailing white space on any lines that this PR
       touches
 - [ ] I have used spaces for indentation on any lines that this PR
       touches
 - [ ] I have included some comments to explain non-obvious code
       changes
 - [ ] I have run the tests localy (`ctest`) and all tests pass
 - [ ] Each commit is a logically atomic, self-consistent, cohesive
       set of changes
 - [ ] The [commit message] should follow [these guidelines]:
     - [ ] First line is directive phrase, starting with a capitalized
           imperative verb, and is no longer than 50 characters
           summarizing your commit
     - [ ] Next line, if necessary is blank
     - [ ] Following lines are all wrapped at 72 characters and can
           include additional paragraphs, bulleted lists, etc.
     - [ ] Use [Github keywords] where appropriate, to indicate the
           commit resolves an open issue.
 - [ ] I have signed  [Contributor License Agreement (CLA)] by
       clicking the "details" link to the right of the `licence/cla`
       check and following the directions on the CLA assistant webpage
 - [ ] I have ensured that the test coverage hasn't gone down and added new [unit tests] to cover an new code added to the library

## Summary of changes ##

Summarize what you changed

## Rationale for changes ##

Why did you make these changes?

## For contributors and SI team members with code review priviledges ##

 - [ ] I certify that I will wait 24 hours before self-approving via
       [pullapprove comment] or [Github code review] so that someone
       else has the chance to review my proposed changes

[links]:#
[contributing guidelines]: https://github.com/sourceryinstitute/OpenCoarrays/blob/master/CONTRIBUTING.md
[commit message]: https://robots.thoughtbot.com/5-useful-tips-for-a-better-commit-message
[these guidelines]: http://tbaggery.com/2008/04/19/a-note-about-git-commit-messages.html
[Contributor License Agreement (CLA)]: https://cla-assistant.io/sourceryinstitute/OpenCoarrays
[pullapprove comment]: https://pullapprove.com/sourceryinstitute/OpenCoarrays/settings/
[Github code review]: https://help.github.com/articles/about-pull-request-reviews/
[Github keywords]: https://help.github.com/articles/closing-issues-via-commit-messages/
[unit tests]: https://github.com/sourceryinstitute/OpenCoarrays/tree/master/src/tests/unit
[PR response img]: https://img.shields.io/issuestats/p/github/sourceryinstitute/OpenCoarrays.svg?style=flat-square
[coverage]: https://img.shields.io/codecov/c/github/sourceryinstitute/OpenCoarrays/master.svg?style=flat-square

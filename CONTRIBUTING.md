# CONTRIBUTING.md

# Issues
To work on the project you must create issues that describe what you are going<br>
to be be working on, and what you are planning to do on the project<br>
segmented in different tasks (one issue per task).<br>
You must follow this format to create issues :
- brief

You should also add tags to easly know what part of the project is concerned by the changes (core, cli, lib, etc.).<br>
You must add the issue to the Github project.<br>
And you should also update the state of the issue after commiting changes to track the advancement.

---

# Branch
To work on the project you must work on a branch seperated of the Master/Main branch, if you are doing a lot of modification/addition/refactoring you must create a specific branch (if the modification are minors you may work on the `dev` branch) and follow the following branch name format :
- name-of-the-branch

## Name
In your name you should describe as simple as possible what you're adding to the project in your branch and what you will be working on, written in [kebab-case](https://developer.mozilla.org/en-US/docs/Glossary/Kebab_case).<br>
ex: add-project-contributor-documentation

---

# Commit
To contribute on the project you must use this commit format to in your commit messages :
- {PREFIX} brief
- {EXTRA_DESCRIPTION}...

## Prefix
- `[ADD]` : addition to the project (a new feature for exemple)<br>
- `[EDIT]` : edition/refactoring of an existing part of the code<br>
- `[DELETE]` : deletion of something in the project<br>
- `[FIX]` : bug/issue fix

## Brief
In your brief you should describe shortly what your prefixed actions have done.<br>
ex: Add CONTRIBUTING.md to the project

## Description
You can also add a description to your commit messages by adding a new line to your commit message
(in the terminal using the cli it can be done by writing git commit -m "PREFIX brief and leaving the quotes open so when you hit enter it let you write after the line break your extra description.).<br>
ex: \[ADD\] CONTRIBUTING.md to the project\n  Made the addition of a contributor documentation to let people know how to properly contribute to the project and follow the commit guidelines!

---

# Merging
When you're done programming your features on your branch you must then create a pull request on `dev` to merge your branch over the `dev` branch.<br>
The `dev` branch on release of multiple major changes should then be merge on the `main` branch also by doing a pull request to merge `dev` on `main`.<br>
In your pull request you should detail what changes you added, add reviewers and if needed apply changes according to reviews and ask the reviewers to review your pull request again.

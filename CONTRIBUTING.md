# Contributing to WinConveyoR

Thank you for your interest in contributing to WinConveyoR! We welcome contributions from the community and appreciate your effort to make this project better.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Issues](#issues)
- [Branches](#branches)
- [Commits](#commits)
- [Pull Requests](#pull-requests)
- [Code Standards](#code-standards)
- [Testing](#testing)
- [Documentation](#documentation)
- [Community](#community)

## Code of Conduct

By participating in this project, you agree to maintain a respectful and inclusive environment for everyone. Please:

- Be respectful and considerate in your communication
- Welcome newcomers and help them get started
- Accept constructive criticism gracefully
- Focus on what is best for the project and community
- Show empathy towards other community members

## Getting Started

### Prerequisites

Before you start contributing, make sure you have:

- Git installed on your system
- A GitHub account
- Familiarity with the project by reading the README.md
- Development environment set up (see README.md for installation instructions)

### Setting Up Your Development Environment

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/WinConveyoR.git
   cd WinConveyoR
   ```
3. **Add the upstream remote**:
   ```bash
   git remote add upstream https://github.com/StevenGandon/WinConveyoR.git
   ```
4. **Install dependencies** (if applicable):
   ```bash
   # For Python/API development
   pip install -r requirements.txt
   ```
5. **Verify your setup** by building the project:
   ```bash
   ./compile_linux.sh  # On Linux
   ```

## Development Workflow

The typical workflow for contributing is:

1. Create or find an issue to work on
2. Create a new branch from `dev`
3. Make your changes following our code standards
4. Write/update tests for your changes
5. Commit your changes with proper commit messages
6. Push your branch and create a pull request
7. Address review feedback
8. Celebrate when your PR is merged!

## Issues

### Creating Issues

To work on the project, you must create issues that describe what you are going to be working on and what you are planning to do, segmented into different tasks (one issue per task).

#### Issue Format

Your issue should include:

**Title**: Clear and concise description

**Description**:
- **Brief**: Short summary of the issue
- **Context**: Why is this needed?
- **Proposed Solution**: How do you plan to address it?
- **Tasks**: Breakdown of work into checkboxes
  - [ ] Task 1
  - [ ] Task 2
  - [ ] Task 3

**Example Issue**:
```
Title: Add user authentication to API

Brief: Implement JWT-based authentication for API endpoints

Context: Currently, the API has no authentication mechanism, making it 
vulnerable to unauthorized access.

Proposed Solution: Add JWT token authentication using FastAPI security utilities

Tasks:
- [ ] Create authentication middleware
- [ ] Add login endpoint
- [ ] Implement token validation
- [ ] Update API documentation
- [ ] Write unit tests
```

### Issue Labels

Use appropriate tags to categorize your issue:

- `core` - Core functionality changes
- `cli` - Command-line interface
- `lib` - Library (libwconr) changes
- `api` - REST API changes
- `bug` - Bug fixes
- `enhancement` - New features
- `documentation` - Documentation updates
- `good first issue` - Suitable for newcomers
- `help wanted` - Need assistance

### Issue Tracking

- **Add the issue to the GitHub project board**
- **Assign yourself** to the issue when you start working on it
- **Update the issue status** regularly (To Do → In Progress → Done)
- **Reference the issue** in your commits and pull requests using `#issue-number`

## Branches

### Branch Strategy

We use a Git Flow-inspired branching strategy:

- `main` - Production-ready code, stable releases
- `dev` - Development branch, integration of features
- `feature/*` - Individual feature branches
- `bugfix/*` - Bug fix branches
- `hotfix/*` - Urgent production fixes

### Creating Branches

Always create your branch from `dev` (unless it's a hotfix):

```bash
git checkout dev
git pull upstream dev
git checkout -b your-branch-name
```

### Branch Naming Convention

Follow this format for branch names:
```
{type}/{brief-description}
```

**Types**:
- `feature/` - New features or enhancements
- `bugfix/` - Bug fixes
- `hotfix/` - Urgent production fixes
- `docs/` - Documentation changes
- `refactor/` - Code refactoring
- `test/` - Test additions or modifications

**Branch Name Format**: Use [kebab-case](https://developer.mozilla.org/en-US/docs/Glossary/Kebab_case)

**Examples**:
- `feature/add-user-authentication`
- `bugfix/fix-memory-leak-in-conveyor`
- `docs/update-api-documentation`
- `refactor/improve-database-queries`
- `test/add-integration-tests`

### Working on Your Branch

```bash
# Keep your branch updated with dev
git checkout dev
git pull upstream dev
git checkout your-branch-name
git rebase dev

# Or merge if you prefer
git merge dev
```

## Commits

### Commit Message Format

Use this format for all commit messages:

```
{PREFIX} brief description

{OPTIONAL_DETAILED_DESCRIPTION}
```

### Commit Prefixes

- `[ADD]` - Addition to the project (new feature, file, functionality)
- `[EDIT]` - Modification or refactoring of existing code
- `[DELETE]` - Removal of code, files, or features
- `[FIX]` - Bug or issue fixes

### Writing Good Commit Messages

**Brief Description**:
- Start with a prefix in square brackets
- Keep it concise (50 characters or less)
- Use imperative mood ("add feature" not "added feature")
- Don't end with a period

**Detailed Description** (optional but recommended):
- Add a blank line after the brief
- Explain the "what" and "why", not the "how"
- Wrap lines at 72 characters
- Reference issues using `#issue-number`

### Commit Examples

**Simple commit**:
```bash
git commit -m "[ADD] user authentication endpoint"
```

**Commit with description**:
```bash
git commit -m "[FIX] memory leak in conveyor monitoring

Fixed a memory leak that occurred when monitoring multiple conveyors 
simultaneously. The issue was caused by event listeners not being 
properly cleaned up.

Closes #42"
```

**More examples**:
```
[ADD] rate limiting middleware to API
[EDIT] refactor database connection handling
[DELETE] deprecated configuration options
[FIX] null pointer exception in CLI startup
[ADD] integration tests for user endpoints
[EDIT] improve error messages in API responses
```

### Atomic Commits

- Make small, focused commits that do one thing
- Each commit should be a logical unit of change
- Don't mix unrelated changes in a single commit
- Commit often to make code review easier

## Pull Requests

### Creating a Pull Request

When your feature is complete:

1. **Push your branch** to your fork:
   ```bash
   git push origin your-branch-name
   ```

2. **Create a pull request** on GitHub from your branch to `dev`

3. **Fill out the PR template** with:
   - Clear title describing the change
   - Reference to related issue(s)
   - Description of changes made
   - Testing performed
   - Screenshots (if UI changes)
   - Breaking changes (if any)

### Pull Request Template

```markdown
## Description
Brief description of what this PR does

Fixes #(issue number)

## Type of Change
- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update

## Changes Made
- Change 1
- Change 2
- Change 3

## Testing
Describe the tests you ran and how to reproduce them:
- Test 1
- Test 2

## Checklist
- [ ] My code follows the project's code style
- [ ] I have performed a self-review of my code
- [ ] I have commented my code where necessary
- [ ] I have updated the documentation accordingly
- [ ] My changes generate no new warnings
- [ ] I have added tests that prove my fix/feature works
- [ ] New and existing unit tests pass locally
- [ ] I have updated CHANGELOG.md (if applicable)
```

### Pull Request Review Process

1. **Add reviewers**: Tag at least one maintainer
2. **Wait for review**: Be patient, reviews may take time
3. **Address feedback**: 
   - Make requested changes
   - Push new commits to your branch
   - Re-request review when ready
4. **CI/CD checks**: Ensure all automated checks pass
5. **Approval**: Once approved, a maintainer will merge your PR

### After Your PR is Merged

- Delete your feature branch (both locally and remotely)
- Pull the latest `dev` branch
- Thank the reviewers!

## Code Standards

### General Guidelines

- Write clean, readable, and maintainable code
- Follow the existing code style and conventions
- Use meaningful variable and function names
- Add comments for complex logic
- Keep functions small and focused on a single task
- Avoid code duplication (DRY principle)

### Language-Specific Standards

**Python** (for API):
- Follow PEP 8 style guide
- Use type hints where appropriate
- Maximum line length: 100 characters
- Use docstrings for functions and classes

**C/C++** (for library):
- Follow project-specific style guide
- Use consistent indentation (spaces, not tabs)
- Document public APIs thoroughly

**Shell Scripts**:
- Use shellcheck to validate scripts
- Add error handling
- Use meaningful variable names

### Code Formatting

```bash
# Python
black api/ app/
flake8 api/ app/

# C/C++ (if using clang-format)
clang-format -i lib/**/*.cpp
```

## Testing

### Writing Tests

- Write tests for all new features
- Maintain or improve code coverage
- Test both success and failure cases
- Use descriptive test names

### Running Tests

```bash
# Run all tests
pytest

# Run with coverage
pytest --cov=api --cov=app --cov-report=html

# Run specific test file
pytest tests/test_api.py

# Run tests matching a pattern
pytest -k "test_authentication"
```

### Test Organization

```
tests/
├── unit/           # Unit tests
├── integration/    # Integration tests
├── fixtures/       # Test fixtures and data
└── conftest.py     # Pytest configuration
```

## Documentation

### Updating Documentation

When making changes, update relevant documentation:

- **Code comments**: For complex logic
- **Docstrings**: For functions, classes, and modules
- **README.md**: For user-facing features
- **API documentation**: For API changes
- **CHANGELOG.md**: For notable changes

### Documentation Style

- Use clear and concise language
- Provide examples where helpful
- Keep documentation up to date with code changes
- Use proper Markdown formatting

## Community

### Getting Help

- **Discord**: Join our Discord server for real-time chat, questions, and discussions with the team and community
- **GitHub Issues**: Check existing issues before creating new ones for bugs or feature requests
- **GitHub Discussions**: For longer-form discussions and ideas

### Join Our Discord

Our Discord server is the primary place for community interaction:

- Ask questions and get help from maintainers and contributors
- Discuss ideas and features before implementing them
- Report bugs and get quick feedback
- Connect with other contributors
- Stay updated on project news and announcements

**Discord Invite**: [Join WinConveyoR Discord](#)

### Discord Channels

- `#general` - General discussions about the project
- `#help` - Get help with setup, development, or usage
- `#contributions` - Discuss PRs, issues, and contributions
- `#announcements` - Project updates and releases
- `#off-topic` - Casual conversations

### Recognition

All contributors will be:
- Listed in the AUTHORS section of README.md
- Credited in release notes for their contributions
- Recognized in our Discord community with a contributor role
- Appreciated by the entire community!

### Staying Updated

- Watch the repository for updates
- Join our Discord server for real-time announcements
- Participate in code reviews and discussions

---

## Quick Reference

### Essential Commands

```bash
# Setup
git clone https://github.com/YOUR_USERNAME/WinConveyoR.git
git remote add upstream https://github.com/StevenGandon/WinConveyoR.git

# Development cycle
git checkout dev
git pull upstream dev
git checkout -b feature/your-feature
# ... make changes ...
git add .
git commit -m "[ADD] your feature"
git push origin feature/your-feature
# ... create PR on GitHub ...

# Stay updated
git checkout dev
git pull upstream dev
git checkout feature/your-feature
git rebase dev
```

### Commit Prefix Quick Guide

- `[ADD]` - New stuff
- `[EDIT]` - Changed stuff
- `[DELETE]` - Removed stuff
- `[FIX]` - Fixed stuff

---

**Thank you for contributing to WinConveyoR!**

Your contributions help make this project better for everyone. If you have any questions or need help, don't hesitate to reach out to the maintainers or open a discussion.
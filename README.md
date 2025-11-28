# WinConveyoR

WinConveyoR is an advanced package manager for Windows, designed to modernise the software ecosystem and offer an experience similar to Linux managers (apt, pacman, dnf, etc.).

The project aims to fill the gap for a comprehensive, powerful and extensible tool that enables:

- Consistent dependency management,
- The creation, compilation and distribution of packages,
- Strong integration with Windows internal mechanisms,
- Extension via a public API, a low-level library and a community marketplace.

This project is being developed as part of the Technical track of the Epitech Innovative Project and is designed to be highly configurable, secure, modular and scalable for both users and third-party developers.


## Badges

[![GPLv3 License](https://img.shields.io/badge/License-GPL%20v3-yellow.svg)](https://github.com/StevenGandon/WinConveyoR/blob/main/LICENSE/)
![Commits](https://img.shields.io/github/commit-activity/t/StevenGandon/WinConveyoR)
![Issues](https://img.shields.io/github/issues/StevenGandon/WinConveyoR)


## Table of Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [API](#api)
- [Configuration](#configuration)
- [Contributing](#contributing)
- [Support](#support)
- [License](#license)
- [Authors](#authors)


## Features

- No Telemetry
- Cross Platform
- GUI Interface
- Community HUB
- Offline Mode
- Auto Update


## Requirements

#### To Build:
- CMake
- Python 3.12
- Make
- GCC
- PyInstaller
- Node.js
- npm

#### To Run:
- Sudo/Admin Privileges (recommended)
- libcurl
- libc


## Installation

#### For linux :
- Install WinConveyoR via command-line:

  ```bash
  curl "https://github.com/StevenGandon/WinConveyoR/releases/download/main/release_ubuntu.zip" -o "release_ubuntu.zip"
  unzip ./release_ubuntu.zip
  sudo mv libwconr.so /usr/lib/
  sudo mv wcr /usr/bin/
  ```

- Install WinConveyoR by compiling it:

  ```bash
  git clone https://github.com/StevenGandon/WinConveyoR.git
  chmod +x ./compile_linux.sh
  ./compile_linux.sh
  cp ./cli/dist/wcr /usr/bin/
  cp ./lib/libwconr/build/libwconr.so /usr/lib/
  ```

#### For windows :
- Install WinConveyoR via github release

  ```bash
  Download https://github.com/StevenGandon/WinConveyoR/releases/download/main/release_windows.zip
  ```

## Usage


## API


## Configuration


## Contributing

Contributions are always welcome!

See `contributing.md` for ways to get started.

Please adhere to this project's `code of conduct`.


## Support

If you encounter any issues or have questions, join our Discord server where you will find:
- Community support and discussions
- Bug reports and issue tracking
- Feature requests and suggestions
- Documentation and tutorials

**Discord Invite**: [Join WinConveyoR Discord](#)

## License

[GPL v3](https://github.com/StevenGandon/WinConveyoR/blob/main/LICENSE)


## Authors

- [@Steven GANDON](https://www.github.com/StevenGandon)
- [@Thomas VIDAL SAVELLI](https://www.github.com/thomasvsl)

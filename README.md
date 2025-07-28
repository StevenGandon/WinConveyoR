
# WinConveyoR

WinConveyoR project
## Badges

[![GPLv3 License](https://img.shields.io/badge/License-GPL%20v3-yellow.svg)](https://github.com/StevenGandon/WinConveyoR/blob/main/LICENSE/)
![Commits](https://img.shields.io/github/commit-activity/t/StevenGandon/WinConveyoR)
![Issues](https://img.shields.io/github/issues/StevenGandon/WinConveyoR)


## Authors

- [@Steven GANDON](https://www.github.com/StevenGandon)
- [@Erwann TANGUY](https://www.github.com/Erwann9875)
- [@Thomas Vidal Savelli](https://www.github.com/thomasvsl)


## Contributing

Contributions are always welcome!

See `contributing.md` for ways to get started.

Please adhere to this project's `code of conduct`.


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
  cp ./lib/libwconr/libwconr.so /usr/lib/
  ```

#### For windows :
- Install WinConveyoR via github release

  ```bash
  Download https://github.com/StevenGandon/WinConveyoR/releases/download/main/release_windows.zip
  ```

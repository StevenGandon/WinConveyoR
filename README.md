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
- [Privacy Policy](#privacy-policy)
- [Authors](#authors)


## Features

- No Telemetry
- Cross Platform
- GUI Interface
- Community HUB
- Offline Mode
- Auto Update


## Requirements

#### To run
- **Linux:** x86-64, Ubuntu 24.04+ or any distribution with **glibc ≥ 2.39**.
  *Known issue:* the server tools (`wcrmirror`, `wcrmgr`, …) shipped up to
  v1.0.4-dev were built against glibc 2.42 and fail on older systems; `wcr` itself
  is not affected.
- **Windows:** Windows 10 / 11, x64.
- **macOS:** Apple Silicon (arm64).

#### To build from source
- CMake, Make, GCC
- OpenSSL and zlib development headers
- Python 3.12 and PyInstaller
- Internet access during the first build: CMake downloads **curl 8.11.1**
  (curl.se), **cJSON 1.7.18** (GitHub) and **libxml2 2.14.5** (gitlab.gnome.org)
- Node.js + npm (GUI only)


## Installation

Releases are published on the
[Releases page](https://github.com/StevenGandon/WinConveyoR/releases). Current
builds are tagged `vX.Y.Z-dev` and marked as pre-releases, so pick the tag you want
there (`/releases/latest/` does not point to them). The examples below use
`v1.0.4-dev`.

Each release contains:

| File | Content |
|------|---------|
| `WinConveyoR-<version>-linux-amd64.run` | Linux installer (recommended) |
| `WinConveyoR-<version>-windows-amd64-setup.exe` | Windows installer (recommended) |
| `WinConveyoR-<version>-macos-arm64.pkg` | macOS installer |
| `WinConveyoR-linux-amd64.zip`, `WinConveyoR-windows-amd64.zip`, `WinConveyoR-macos-arm64.zip` | Portable archives: `bin/` (wcr + server/dev tools), `lib/` (libwconr + bundled libraries), `include/` |
| `SHA256SUMS.txt` | Checksums |

### Linux

**Installer (recommended):**

```bash
VERSION=1.0.4-dev
curl -LO "https://github.com/StevenGandon/WinConveyoR/releases/download/v$VERSION/WinConveyoR-$VERSION-linux-amd64.run"
sh "WinConveyoR-$VERSION-linux-amd64.run" --accept-license
```

It installs to `/usr/local` when run as root, `~/.local` otherwise. Useful options:
`--prefix DIR`, `--components cli,server,devtools` (or `all`), `--no-modify-path`,
`--uninstall`. Run with `--help` for details.

**Portable archive:**

```bash
VERSION=1.0.4-dev
curl -LO "https://github.com/StevenGandon/WinConveyoR/releases/download/v$VERSION/WinConveyoR-linux-amd64.zip"
unzip WinConveyoR-linux-amd64.zip -d WinConveyoR
mkdir -p ~/.local/bin && cp WinConveyoR/bin/* ~/.local/bin/   # make sure ~/.local/bin is in your PATH
wcr --help
```

`wcr` is self-contained; `lib/` and `include/` are only needed to build against
libwconr. To verify a download: `sha256sum -c --ignore-missing SHA256SUMS.txt`.

**From source:**

```bash
sudo apt install cmake gcc g++ make libssl-dev zlib1g-dev python3 python3-venv
git clone https://github.com/StevenGandon/WinConveyoR.git
cd WinConveyoR
python3 -m venv .venv && . .venv/bin/activate && pip install pyinstaller
bash scripts/lib/build_linux.sh    # -> dist/linux-amd64/lib/libwconr.so
bash scripts/cli/build_linux.sh    # -> dist/linux-amd64/bin/wcr
./dist/linux-amd64/bin/wcr --help
```

Run the scripts with `bash`, not `sh`. Optional: `scripts/server/build_linux.sh`
(mirror tools) and `scripts/devtools/build_linux.sh`. All outputs go to
`dist/linux-amd64/`.

### Windows

**Installer (recommended):** download
`WinConveyoR-<version>-windows-amd64-setup.exe` from the
[Releases page](https://github.com/StevenGandon/WinConveyoR/releases) and run it.

**Portable archive:** download `WinConveyoR-windows-amd64.zip`, extract it, and
run `bin\wcr.exe --help` (add `bin\` to your `PATH` to use `wcr` anywhere).

**From source:** requires [MSYS2](https://www.msys2.org/) installed in
`C:\msys64` with the MinGW-w64 toolchain and OpenSSL, CMake, Python 3.12 and
PyInstaller, all in your `PATH`. Then, from the repository root:

```bat
scripts\lib\build_windows.bat
scripts\cli\build_windows.bat
```

Outputs go to `dist\windows-amd64\`.

### macOS

Download the `.pkg` installer or `WinConveyoR-macos-arm64.zip` from the
[Releases page](https://github.com/StevenGandon/WinConveyoR/releases).


## Usage

```text
wcr <command> [package] [options]
```

| Command | Description |
|---------|-------------|
| `update` (`-u`) | Sync package lists from the configured sources |
| `search <name>` | Search available packages |
| `info <name>` | Show package variants |
| `install <name>` (`-i`) | Install a package |
| `uninstall <name>` | Uninstall a package |
| `list` | List installed packages |
| `upgrade [name]` | Upgrade all packages, or a specific one |
| `check <name>` | Verify the cached package hash |
| `register` / `login` | Create an account / log in |
| `help` (`-h`) | Show help |

Display options: `--no-color`, `--no-ansi`, `--ascii`, `-ts/--terminal-support`,
`-chst/--charset`. When the output is not a terminal (pipe, redirect),
`search`, `list` and `info` print JSON instead of a table.

### First `wcr update`

`wcr` downloads packages from **sources** (mirrors). Without configuration it
tries, in this order, a mirror on your own machine over HTTP
(`http://localhost:8080`), then over the WCR protocol (`localhost:1674`). To try
WinConveyoR, start a local WCR mirror with the example packages shipped in the
repository:

```bash
git clone https://github.com/StevenGandon/WinConveyoR.git
cd WinConveyoR/hosting/mirror-server
python3 -m venv .venv && . .venv/bin/activate
pip install -r requirements.txt
cp -r ../mirror-example /tmp/wcr-mirror
WCR_DIR=/tmp/wcr-mirror python3 wcrmirror.py      # listens on port 1674
```

Then, in another terminal:

```console
$ wcr update
wcr update: syncing from http://localhost:8080...
wcr update: sync failed for http://localhost:8080 (rc=-1).
wcr update: syncing from localhost:1674...
wcr update: ok.
```

The `sync failed` line for port 8080 is expected: only the WCR mirror is running.
The update succeeds as long as one source answers.

```console
$ wcr search gcc
  gcc                             1.0.0

1 package(s) available.
```

To use another mirror, add it as a source (see [Configuration](#configuration)).


## API


## Configuration

The client reads its configuration from **`~/.config/wcr/config`**. If the file
does not exist, `wcr` uses the two default sources above; the file is written
after the first `wcr update` in which at least one source succeeds.

Format — one `key=value` per line, one `source=` line per mirror, tried in order:

```ini
cache_path=/home/<user>/.cache/wcr
source=http http://localhost:8080
source=wcr localhost:1674
```

| Key | Meaning |
|-----|---------|
| `cache_path` | Where downloaded packages are cached |
| `source=<protocol> <address>` | A mirror. `<protocol>` is `http` (URL) or `wcr` (`host:port`). |
| `access_key=<key>` | *(optional)* Access key for the source on the line above, if the mirror requires one |
| `server_pubkey=<path>` | *(optional)* Path to the mirror's public RSA key, for the source on the line above |


## Contributing

Contributions are always welcome! See [`CONTRIBUTING.md`](CONTRIBUTING.md) for
ways to get started.


## Support

If you encounter an issue or have a question, open an issue on
[GitHub Issues](https://github.com/StevenGandon/WinConveyoR/issues) — bug reports,
feature requests and questions are all welcome.

## License

[GPL v3](https://github.com/StevenGandon/WinConveyoR/blob/main/LICENSE)

## Privacy Policy

See our complete [Privacy Policy](https://github.com/StevenGandon/WinConveyoR/blob/dev/doc/privacy_policy.md) for full details on data collection and your rights.


## Authors

- [@Steven GANDON](https://www.github.com/StevenGandon)
- [@Thomas VIDAL SAVELLI](https://www.github.com/thomasvsl)

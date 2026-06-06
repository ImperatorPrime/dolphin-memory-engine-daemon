# dolphin-memory-engine-daemon
A daemon for running Dolphin in Randovania as a Nintendont connection.

It connects directly with Dolphin through dolphin-memory-engine so that Randovania itself does not need to run
as root to connect to Dolphin. It does this by posing as a fully compliant Nintendont client that forwards all commands it receives to Dolphin.

## Installation

### Debian / Ubuntu (.deb)

Download the latest `.deb` from the [releases page](../../releases/latest) and install it:

```bash
sudo apt install ./dolphin-memory-engine-daemon-<version>-Linux.deb
```

### Fedora / RHEL / Rocky Linux (.rpm)

Download the latest `.rpm` from the [releases page](../../releases/latest) and install it:

```bash
sudo rpm -i dolphin-memory-engine-daemon-<version>-Linux.rpm
```

### Other Linux (.tar.gz)

Download the latest `.tar.gz` from the [releases page](../../releases/latest), extract it, and run the install script:

```bash
tar xzf dolphin-memory-engine-daemon-<version>-linux.tar.gz
cd dolphin-memory-engine-daemon-<version>-linux
sudo ./install.sh
```

The install script must be run as root as it installs files to `/usr/bin` and `/lib/systemd/system`. It will start the daemon immediately and prompt you to enable it on boot.

## Running

If the daemon is not already running, start it:

```bash
sudo systemctl start dolphin-memory-engine-daemon
```

Then in Randovania, open a new game connection, add a new Nintendont connection, and enter `127.0.0.1` as the IP address.

Other useful commands:

```bash
# Stop
sudo systemctl stop dolphin-memory-engine-daemon

# Enable on boot
sudo systemctl enable dolphin-memory-engine-daemon

# Disable on boot
sudo systemctl disable dolphin-memory-engine-daemon

# Check status
systemctl status dolphin-memory-engine-daemon

# View logs
journalctl -u dolphin-memory-engine-daemon
```

## Building from Source

### Dependencies

- CMake 3.15+
- Boost
- libcap

On Debian/Ubuntu:
```bash
sudo apt install cmake libboost-dev libcap-dev
```

On Fedora/RHEL/Rocky Linux:
```bash
sudo dnf install cmake boost-devel libcap-devel
```

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Install

```bash
sudo cmake --install build
sudo cp dist/dolphin-memory-engine-daemon.service /lib/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now dolphin-memory-engine-daemon
```

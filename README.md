# EternalPatcherLinux
![Build Status](https://github.com/PowerBall253/EternalPatcherLinux/actions/workflows/cmake.yml/badge.svg)

DOOM Eternal executable patcher, rewritten in C for Linux. 

## Patches
The patches are defined in a patch definitions file (EternalPatcher.def), which is downloaded by the tool using the update server specified in the configuration file (EternalPatcher.config). For more information, check out the original EternalPatcher [here](https://github.com/dcealopez/EternalPatcher). The default config file should look like this:
```
UpdateServer = "dcealopez.es";
```

## Usage
```
./EternalPatcher [--update] [--patch /path/to/DOOMEternalx64vk.exe]
```
* --update - Checks for updates and downloads them if available.

* --patch - Patches the given game executable using the downloaded patch definitions.

## Compiling
The project uses Cmake to compile, and requires curl and OpenSSL to be installed.

First clone the repo by running:

```
git clone https://github.com/PowerBall253/EternalPatcherLinux.git
```

Then, generate the makefile by running:
```
cd EternalPatcherLinux
mkdir build
cd build
cmake ..
```

Finally, build with:
```
make
```

The EternalPatcher executable will be in the "build" folder.

## Credits
* proteh: For creating the original EternalPatcher.

# EternalPatcherGo
![Build Status](https://github.com/PowerBall253/EternalPatcherGo/actions/workflows/test.yml/badge.svg)

DOOM Eternal executable patcher, rewritten in Go.

## Patches
The patches are defined in a patch definitions file (EternalPatcher.def), which is downloaded by the tool using the update server specified in the configuration file (EternalPatcher.config). For more information, check out the original EternalPatcher [here](https://github.com/dcealopez/EternalPatcher). The config file uses the JSON format, and it should look like this:

```
{
    "updateServer" : "dcealopez.es"
}
```

## Usage

```
EternalPatcher [--update] [--patch /path/to/DOOMEternalx64vk.exe]
```

* `--update` - Checks for updates and downloads them if available.

* `--patch` - Patches the given game executable using the downloaded patch definitions.

## Compiling
The project requires the [go toolchain](https://go.dev/dl/) to be compiled.

To compile, run:

```
go build -o EternalPatcher -tags netgo -ldflags="-s -w" .
```

To set a version number, build with:

```
go build -o EternalPatcher -tags netgo -ldflags="-s -w -X 'main.Version=vX.Y.Z'" .
```

(replace vX.Y.Z with the version number you prefer).

Additionally, you may use [UPX](https://upx.github.io/) to compress the binary:

```
upx --best EternalPatcher
```

## Credits
* proteh: For creating the original EternalPatcher.

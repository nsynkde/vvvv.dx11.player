# vvvv.player

Supports image sequence streaming from disk to GPU. Supported file formats are:
- `.tga`
- `.dds`
- `.dpx`

### Building
Open `DX11Player.sln` from `plugins/DX11Player` and build the solution file.

### Debugging
For debugging the example patch, build the `Native` project with `Release`
configuration first, then run the `DX11Player` project in Debug mode.

### Installation
Build with configuration `Release` and copy `build\Release\nsynk` into your
`%VVVV%/packs` directory.

### Structure:
`Dependencies` servers the following third party libraries, because NuGet packages seem to be out of date for these:
- FeralTic.dll
- VVV.DX11.Core.dll
- VVV.DX11.Lib.dll

### Example
1. Extract `data\bick_buck_bunny.7z` to `data\bick_buck_bunny`
2. Open `patches/nsynk.player.example.v4p`

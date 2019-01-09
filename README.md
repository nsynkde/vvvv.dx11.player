# vvvv.player

Supports image sequence streaming from disk to GPU. Supported file formats are:
- `.tga`
- `.dds`
- `.dpx`

### Building
Open `DX11Player.sln` from `plugins/DX11Player` and build the solution file.

### Installation
Build with configuration `Release` and copy `build\Release\nsynk` into your
`%VVVV%/packs` directory.

### Structure:
`Dependencies` servers the following third party libraries, because NuGet packages seem to be out of date for these:
- FeralTic.dll
- VVV.DX11.Core.dll
- VVV.DX11.Lib.dll

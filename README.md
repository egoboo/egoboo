# Egoboo

Egoboo is a working cool 3D dungeon crawling game in the spirit of NetHack.
It supports Windows, Linux and Mac.

### License
Egoboo is made available publicly under the
[GNU GPLv3 License](https://github.com/egoboo/egoboo/LICENSE).

### Contact
Developers can usually be contacted via GitHub.

### Building from Source
Checkout the repository 
```
git clone https://github.com/egoboo/egoboo
```
into some directory called the *source directory*.

Change into the *source directory* and initialize and update the submodules
```
git submodule update --init --recursive
```
Next, perform an out of source build by creating a directory called the *build directory*.
The *build directory* must reside outside of the *source directory*.
Change into the *buid directory* and enter
```
cmake <path to source directory>
```
where `<path to source directory>` is replaced by the actual path to your *source directory*.
CMake will generate the environment specific build files (e.g., Visual Studio files, Make files, etc.) in the *build directory*.

#### Appveyor CI Build Status
Appveyor CI build status of [master](https://github.com/egoboo/egoboo/tree/master) branch:
[![Build status](https://ci.appveyor.com/api/projects/status/7sjmdgolmvmv3hc1/branch/master?svg=true)](https://ci.appveyor.com/project/michaelheilmann-com/egoboo/branch/master)

# osu!performance
[![Travis Build Status](https://travis-ci.org/ppy/osu-performance.svg?branch=master)](https://travis-ci.org/ppy/osu-performance)
[![Appveyor Build status](https://ci.appveyor.com/api/projects/status/4xvd8p8ulci07d82?svg=true)](https://ci.appveyor.com/project/Tom94/osu-performance)
[![dev chat](https://discordapp.com/api/guilds/188630481301012481/widget.png?style=shield)](https://discord.gg/ppy)

This is the program computing "performance points" (pp), which are used as the official [player ranking metric](https://osu.ppy.sh/p/pp) in osu!.

## Compiling

osu!performance runs on Windows and Linux. It should also compile and run on macOS, but this has not been tested. The build environment is set up using [CMake](https://cmake.org/) as follows.

### Windows

Open the command line and navigate to the root folder of this repository.

```sh
osu-performance> mkdir Build
osu-performance> cd Build
osu-performance\Build> cmake ..
```

Now the _Build_ folder should contain a [Visual Studio](https://www.visualstudio.com/) project for building the program.

### macOS / Linux

On macOS / Linux you need to install the [MariaDB](https://mariadb.org/) MySQL connector and [cURL](https://curl.haxx.se/) packages. Afterwards, in a terminal of your choice, do

```sh
osu-performance$ mkdir Build
osu-performance$ cd Build
osu-performance/Build$ cmake ..
osu-performance/Build$ make -j
```

## Usage

After compilation, an executable named `osu-performance` is placed in the _Bin_ folder. It accepts the following arguments:
* `-m <id>`
  
  This option controls the gamemode for which pp is computed. __id__ can have the following values:

  __Standard__ | __Taiko__ | __Catch the beat__ | __osu!mania__
  --- | --- | --- | ---
  0 | 1 | 2 | 3
* `-r`
  
  If this option is present, then the pp values of all existing scores are re-computed. Otherwise, only new pp values are computed.

* `-f`
  
  If this option is not present, then a child process is created, which starts this program a second time as a child process with the same options and -f enabled additionally. Whenever this child process terminates, after 5 seconds it is restarted. The purpose of this option is to ensure, that crashes and errors do not interrupt the computation of pp values.

Configuration options beyond these parameters, such as the MySQL server configuration, can be adjusted in _Bin/Data/Config.cfg_.

## Licence

osu-performance is licensed under AGPL version 3 or later. Please see [the licence file](LICENCE) for more information. [tl;dr](https://tldrlegal.com/license/gnu-affero-general-public-license-v3-(agpl-3.0)) if you want to use any code, design or artwork from this project, attribute it and make your project open source under the same licence.

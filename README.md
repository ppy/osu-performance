# osu!performance ![](https://github.com/ppy/osu-performance/workflows/CI/badge.svg) [![dev chat](https://discordapp.com/api/guilds/188630481301012481/widget.png?style=shield)](https://discord.gg/ppy)

This is the program computing "performance points" (__pp__), which are used as the official [player ranking metric](https://osu.ppy.sh/p/pp) in osu!.

# Current Versions

The `master` branch of this repository tracks ongoing developments, and may be newer than what is used for the live osu! systems. If looking to use the correct version for matching live values, the correct version is:

| `osu-performance` |
| -- |
| [2022.929.0](https://github.com/ppy/osu-performance/releases/tag/2022.929.0) |

# Compiling

All that is required for building osu!performance is a C++11-compatible compiler. Begin by cloning this repository and all its submodules using the following command:
```sh
$ git clone --recursive https://github.com/ppy/osu-performance
```

If you accidentally omitted the `--recursive` flag when cloning this repository you can initialize the submodules like so:
```sh
$ git submodule update --init --recursive
```

osu!performance runs on Windows, macOS, and Linux. The build environment is set up using [CMake](https://cmake.org/) as follows.

### Windows

Open the command line and navigate to the root folder of this repository.

```sh
osu-performance> mkdir build
osu-performance> cd build
osu-performance\build> cmake ..
```

Now the _build_ folder should contain a [Visual Studio](https://www.visualstudio.com/) project for building the program. Visual Studio 2017 and a 64-bit build are recommended (`cmake -G "Visual Studio 15 2017 Win64" ..`).

### macOS / Linux

On macOS / Linux you need to install the [MariaDB](https://mariadb.org/) MySQL connector and [cURL](https://curl.haxx.se/) packages. Afterwards, in a terminal of your choice, do

```sh
osu-performance$ mkdir build
osu-performance$ cd build
osu-performance/build$ cmake ..
osu-performance/build$ make -j
```

# Sample Data

Database dumps with sample data can be found at https://data.ppy.sh. This data includes the top 10,000 users along with a random 10,000 user sample across all users, along with all required auxiliary tables to test this system. Please note that this data is released for development purposes only (full licence details [available here](https://data.ppy.sh/LICENCE.txt)).

You can import these dumps to mysql (after first extracting them) by running `cat *.sql | mysql`. Note that all existing data in tables will be dropped and replaced. Make sure to import the latest available data dumps as older snapshots may be incompatible with the latest version of osu!performance.

# Usage

First, [set up a MySQL server](https://dev.mysql.com/doc/mysql-getting-started/en/) and import the provided data from above which is most relevant to your use case. Next, edit _bin/config.json_ with your favourite text editor and configure `mysql.master` to point to your MySQL server.

After compilation, an executable named `osu-performance` is placed in the _bin_ folder. You can use it via the command line as follows:

```sh
./osu-performance COMMAND {OPTIONS}
```

where command controls which scores are the target of the computation.
The following commands are valid:
* `all`: Compute pp of all users
* `new`: Continually poll for new scores and compute pp of these
* `scores`: Compute pp of specific scores
* `users`: Compute pp of specific users
* `sql`: Compute pp of users given by a SQL select statement

The gamemode to compute pp for can be selected via the `-m` option, which may take the value `osu`, `taiko`, `catch`, or `mania`.

Information about further options can be queried via

```sh
./osu-performance -h
```

and further options specific to the chosen command can be queried via

```sh
./osu-performance COMMAND -h
```

Configuration options beyond these parameters, such as various API hooks, can be adjusted in _bin/config.json_.

# Docker

osu!performance can also be run in Docker.

Configuration is provided via environment variables _or_ by mounting the config file at _/srv/config.json_.
Availables environment variables:
```
MYSQL_HOST
MYSQL_PORT
MYSQL_USER
MYSQL_PASSWORD
MYSQL_DATABASE

MYSQL_SLAVE_HOST
MYSQL_SLAVE_PORT
MYSQL_SLAVE_USER
MYSQL_SLAVE_PASSWORD
MYSQL_SLAVE_DATABASE

MYSQL_USER_PP_TABLE_NAME
MYSQL_USER_METADATA_TABLE_NAME

WRITE_ALL_PP

POLL_INTERVAL_DIFFICULTIES
POLL_INTERVAL_SCORES

SENTRY_HOST
SENTRY_PROJECTID
SENTRY_PUBLICKEY
SENTRY_PRIVATEKEY

DATADOG_HOST
DATADOG_PORT
```

Example:
```sh
docker build -t osu-performance .
docker run --rm -it \
  -e MYSQL_HOST=172.17.0.1 \
  -e MYSQL_USER=osu \
  -e MYSQL_PASSWORD=changeme \
  -e MYSQL_DATABASE=osu \
  osu-performance all -m osu
```

A `docker-compose.yml` file is also provided, with a built-in MySQL and phpMyAdmin server provided for convenience.
It supports having *.sql files in a dump folder, such as those found at https://data.ppy.sh, for import at first start.

# Licence
osu!performance is licensed under AGPL version 3 or later. Please see [the licence file](LICENCE) for more information. [tl;dr](https://tldrlegal.com/license/gnu-affero-general-public-license-v3-(agpl-3.0)) if you want to use any code, design or artwork from this project, attribute it and make your project open source under the same licence.

Note that the sample data is covered by a [separate licence](https://data.ppy.sh/LICENCE.txt).

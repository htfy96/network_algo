#Network Algorithm
This project is a framework of network analysis. It is designed to provide the following features:
 - Building graph from various sources(e.g MYSQL)
 - Implementation of some network algorithms.

It is still at very early stage, so use it as your own risk.

##Clone & Build
```
git clone --recursive https://github.com/htfy96/network_algo.git
cd network_algo
mkdir build
cd build
cmake ..
make
```

##Depends
 - `boost::graph`
 - `soci` (as a submodule)
 - the library files of included database(s) (e.g `mysql.h` if you want to use mysql)

##Tech Stack
 - TODO `boost::graph`
 - `soci` (C++ database library)

##LICENSE
GPLv3

 > This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 > This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 > You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

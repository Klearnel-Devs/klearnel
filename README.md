Klearnel
========

What is it?
-----------

A Linux kernel autocleaning module allowing for an intervention-free, clean filesystem. Klearnel is designed first for servers but can be used on workstations.

How to build it
---------------

### Prerequisites

First of all, you need to clone this project with `git clone https://github.com/Klearnel-Devs/klearnel.git`.

Once this step is complete, you will need to install the openssl library. Below you will find how to install it for several operating systems:

* For Debian based systems: `apt-get install libssl-dev -y`
* For Red Hat based systems: `yum install openssl-devel -y`
* For Mac OS: 
	* You will need the command-line tool called "Homebrew". If you do not have it, please follow the instructions [here](http://brew.sh/).
	* Issue the following command: `brew install openssl`

### Build

To build the project, navigate to the Klearnel folder where you cloned this git.

For example: `cd ~/klearnel`

Use the command `make`. If this step returns an error `#include <openssl/sha.h>: File Not Found`, please verify that you have done the Prerequisites part of this Readme.

### Install

Once the build part complete, install Klearnel using this command: `make install`.

You will need to do this as root.

### Generate the documentation

If you want to generate the documentation of the source code, you will need doxygen. 

Once doxygen installed on your system, do this in the folder of the project: `doxygen doxy.conf`.

The generated documentation will be available at `build/bin/doc`.

How to contribute
-----------------

Any contribution is welcome. To help the project to evolve, please fork this repo and make a PR with your changes.

Please notice that we use Doxygen to comment the source code. If you want to learn more about it, visit [this](https://en.wikipedia.org/wiki/Doxygen).


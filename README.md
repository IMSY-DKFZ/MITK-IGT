MITK-IGT
========

This project is an extension of [MITK](https://github.com/MITK/MITK) providing image-guided therapy functionality. Until March 2024, the project was managed in a joint repository with MITK, which is still included in the commit history. Since March 2024, it can be integrated as an extension.

Build instructions
------------
#### Note on build configurations, tools and compilers:

We currently recommend the following configuration for compiling MITK with IGT extension: 
- MITK in [version 2024.06.2](https://github.com/MITK/MITK/releases/tag/v2024.06.2) 
- Windows 10 or 11 as operating system, 
- Visual Studio 2019 as compiler, 
- Qt library in version 6.6.2 (activate archive during Qt installation) and 
- Cmake in [version 3.27.9](https://cmake.org/files/v3.27/cmake-3.27.9-windows-x86_64.msi). 

Please note that there is a list of supported platforms for each MITK version (link to [Supported Platforms for MITK 2024.06](https://docs.mitk.org/2024.06/SupportedPlatformsPage.html)), but due to the additional libraries and hardware connections, not all of the configurations specified there may also work with the IGT extension.

#### Note on the base MITK:

Before the IGT extension is integrated into a build, it would be helpful if MITK compiles without any problems. It is therefore recommended that you first build MITK without the IGT extension in the appropriate version and with the build tools you plan to use (see [MITK Build Instructions for MITK 2024.06](https://docs.mitk.org/2024.06/BuildInstructionsPage.html)).

#### Approach to build MITK with IGT extension:
1. Clone [MITK](https://github.com/MITK/MITK) and switch to release tag (e.g., [v2024.06.2](https://github.com/MITK/MITK/releases/tag/v2024.06.2))
2. Clone this repository
3. Using CMake, configure MITK and set `MITK_EXTENSION_DIRS` to your working copy of this repository
4. Configure (and generate) again and make sure that `MITK_BUILD_CONFIGURATION` is set to `mitkNavigationModules`
5. Build MITK

Documentation
------------
The latest documentation of MITK-IGT can be found [ḥere](https://imsy-dkfz.github.io/MITK-IGT/). If you are interested in the documentation of the last 2023.12 release (in which MITK-IGT was still included in the MITK repository) refer to [here](https://docs.mitk.org/2023.12/IGTConcepts.html).
To build the documentation for IGT by yourself, install [doxygen](https://doxygen.nl/index.html) and run `doxygen Documentation/local-doxygen.conf` from the root folder of this repo. This will generate a html version of the documentation that can be opened via the index.html file.

License
-------

MITK-IGT is available as free open-source software under a [3-clause BSD license](LICENSE). Copyright (c) [German Cancer Research Center (DKFZ)](https://www.dkfz.de) All rights reserved.

MITK-IGT is published by the [Division of Intelligent Medical Systems](https://github.com/IMSY-DKFZ) of DKFZ. 

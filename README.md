MITK-IGT
========

This project is an extension of [MITK](https://github.com/MITK/MITK) providing image-guided therapy functionality. Until March 2024, the project was managed in a joint repository with MITK, which is still included in the commit history. Since March 2024, it can be integrated as an extension.

How it works
------------

1. Clone [MITK](https://github.com/MITK/MITK)
2. Clone this repository
3. Using CMake, configure MITK and set `MITK_EXTENSION_DIRS` to your working copy of this repository
4. Configure (and generate) again and make sure that `MITK_BUILD_CONFIGURATION` is set to `mitkNavigationModules`
5. Build MITK

Documentation
------------
Documentation is included in the source code repository (see [here](https://github.com/IMSY-DKFZ/MITK-IGT/tree/main/Modules/IGT/Documentation/doxygen)). Please build doxygen for a more reader-friendly form. A built documentation of the last 2023.12 release (in which MITK-IGT was still included in the MITK repository) is available [here](https://docs.mitk.org/2023.12/IGTConcepts.html).
To build the documentation for IGT by yourself, install [doxygen](https://doxygen.nl/index.html) and run `doxygen Documentation/local-doxygen.conf` from the root folder of this repo. This will generate a html version of the documentation that can be opened via the index.html file.

License
-------

MITK-IGT is available as free open-source software under a [3-clause BSD license](LICENSE). Copyright (c) [German Cancer Research Center (DKFZ)](https://www.dkfz.de) All rights reserved.

MITK-IGT is published by the [Division of Intelligent Medical Systems](https://github.com/IMSY-DKFZ) of DKFZ. 

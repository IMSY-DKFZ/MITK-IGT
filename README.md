MITK-IGT
========

This project is an extension of [MITK](https://github.com/MITK/MITK) providing image-guided therapy functionality.

How it works
------------

1. Clone [MITK](https://github.com/MITK/MITK)
2. Clone this repository
3. Using CMake, configure MITK and set `MITK_EXTENSION_DIRS` to your working copy of this repository
4. Configure (and generate) again and make sure that `MITK_BUILD_CONFIGURATION` is set to `mitkNavigationModules`
5. Build MITK

License
-------

MITK-IGT is licensed under a [3-clause BSD license](LICENSE).

Until and including February 2024, MITK-IGT was an integral part of MITK. The following copyright applies up to this point accordingly:

Copyright (c) [German Cancer Research Center (DKFZ)](https://www.dkfz.de)<br>
All rights reserved.

Support and maintenance
-----------------------

Since March 2024, MITK-IGT is officially maintained by [@Alfred-Franz](https://github.com/Alfred-Franz).

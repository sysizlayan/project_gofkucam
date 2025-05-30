[![Getting Started with QP Video](https://www.state-machine.com/img/YT-Getting_Started_with_QP.jpg)](https://youtu.be/O7ER6_VqIH0 "Getting Started with QP")

---------------------------------------------------------------------------
# About QP-bundle
QP-bundle provides all QP real-time embedded frameworks (RTEFs) and
all the accompanying host-based tools bundled together in a single,
streamlined installation.

## Downloading QP-bundle
You downlod QP-bundle from Quantum Leaps:

- https://www.state-machine.com/#Downloads


After you donwload and install the QP-bundle release specific for your
host operating system (Windows, Linux or MacOS), the installation
directory on your disk will contain the following components:

```
qp-bundle installation directory (C:\qp or $HOME\qp by default)
|
+-qpc\      - QP/C    real-time embedded framework (RTEF)
+-qpcpp\    - QP/C++  real-time embedded framework (RTEF)
|
+-qm\       - QM model-based design tool
|
+-qtools\   - QTools collection
| +-bin\    - QTools executables
| +-qclean\ - Source code white space cleaner
| +-qspy\   - QSPY host application
| +-qutest\ - QUTest unit testing harness
| +-qview\  - QView visualization and monitoring
| |
(the following subfolders are present on Windows only)
| |
| +-mingw32\           - GNU C/C++ toolchain for Windows (mingw-32-bit)
| +-gnu_arm-none-eabi\ - GNU-ARM C/C++ toolset for ARM Cortex-M/R
| +-Python38\          - Python 3.8 (for QUTest and QView)
```

> NOTE: A description of each component is provided in the README.md
files inside the project sub-directories.

---------------------------------------------------------------------------
### Version Compatibility and Revision History

The version of each component is encoded in the file "version-X.Y.Z",
which is provided in the component sub-directory.

The revision history of each component is provided in the Reference
Manual for each component (see the next section). 

---------------------------------------------------------------------------
### Reference Manuals
- https://www.state-machine.com/qpc     - QP/C Reference Manual
- https://www.state-machine.com/qpcpp   - QP/C++ Reference Manual
- https://www.state-machine.com/qm      - QM Reference Manual
- https://www.state-machine.com/qtools  - QTools Reference Manual

---------------------------------------------------------------------------
### Help and Support
Community support for the project is provided at:
- https://sourceforge.net/p/qpc/discussion/668726

Commercial technical support is described at:
- https://www.state-machine.com/licensing/#Support

---------------------------------------------------------------------------
### Bug Reports and Feature Requests
- Bug reports:      https://sourceforge.net/p/qpc/bugs
- Feature requests: https://sourceforge.net/p/qpc/feature-requests/

Please use the "Create Ticket" menu.

---------------------------------------------------------------------------
### Contact Information:
- https://www.state-machine.com
- info@state-machine.com


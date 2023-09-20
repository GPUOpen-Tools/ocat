Changelog
=========

1.6.3 - 2023-09-13
----------------

Changed
^^^^^^^

- Updated the Platform Toolset to v143 (Visual Studio 2022)
- Updated the .net version to 4.7.2

Fixed
^^^^^

- Fixed a bug that affected overlay compatibility with a number of games

1.6.2 - 2021-12-21
----------------

Added
^^^^^

- Option to set working directory when launching app from within OCAT

Changed
^^^^^^^

- Updated Windows SDK and Vulkan SDK
- Updated GPUDetect

Fixed
^^^^^

- Fixed several bugs that affected overlay compatibility with a number of games

1.6.1 - 2020-12-07
----------------

Added
^^^^^

- Support for RDNA and RDNA2 GPUs

  - Updated AGS to v6.0

- Resolution information gets also retrieved in window mode now

Fixed
^^^^^

- Fixed several bugs


1.6 - 2019-12-10
----------------

Added
^^^^^

- Option to disable the overlay while taking a capture

  - Reduces the overhead of the overlay to get more accurate capture results

Fixed
^^^^^

- Fixed some compatibility issues

1.5.1 - 2019-07-25
------------------

Fixed
^^^^^

- DX overlay compatibility issues

1.5 - 2019-06-25
----------------

Added
^^^^^

- Lag indicator overlay

  - Mutually exclusive to rest of overlay
  - Changes color on lag indicator hotkey signal

- Estimated driver lag metric in performance summary and verbose log files
- Resolution information in performance summary and verbose log files
- Audible indicator toggle
- Option to require ALT + hotkey combination instead of single hotkey only

Changed
^^^^^^^

- UI has now a latency tab with the lag indicator hotkeys and the FCAT-style per-frame coloured bar visibility hotkey

Fixed
^^^^^

- Fixed some hotkey issues causing it to not work correctly with a bunch of games
- Vulkan overlay compatibility with other Vulkan layers
- Proper usage of user blacklist

1.4 - 2019-03-15
----------------

Notes
^^^^^

- New version numbering scheme: major.minor.build (used to be major.minor.0.build)

  - That pesky zero broke things so we nuked it from orbit

Added
^^^^^

- Audible indicators for starting and stopping recording

  - Helps when the overlay isn't compatible or available

- Overlay now prints the graphics API being used
- Rolling plot of frame times added to overlay
- 95th and 99.9th percentile frame times in the performance summary
- FCAT-style per-frame coloured bar

Changed
^^^^^^^

- Hotkey hooking method changed to Windows' global hooking

Fixed
^^^^^

- Windows 7 compatibility
- Destination folder for captures is properly remembered between launches
- Properly deactivate the Vulkan overlay layer on OCAT crash

1.3.0 - 2018-12-13
------------------

Notes
^^^^^

- Please make sure to uninstall any previous release before installing 1.3.0

  - There is a known issue that prevents the installer from succeeding that will be fixed in a future release!

Added
^^^^^

- Reworked UI

  - Changed to make it much more intuitive to use

    - New Overlay, Capture, Launch App and Visualize tabs
    - Overlay tab controls overlay configuration
    - Capture tab controls capture settings
    - Launch App tab controls overlay injection settions for a single application
    - Visualize tab launches the visualizer!
    - Read `OCAT usage <usage.html>`_ for more detailed information

- User-supplied blacklist

  - OCAT maintains its own internal blacklist, but also lets you provide your own

    - That ensures OCAT doesn't remove your own blacklist settings when OCAT is updated
    - See `Blacklist <usage.html#blacklist>`_ for more detailed information

- Launching Steam apps via Steam AppId

  - Allows OCAT to work with many more Steam games and lets the explicit hook work in more cases
  - Read `Launch App <usage.html#launch-app>`_ for more detailed information

Changed
^^^^^^^

- System specifications added to the first line of the verbose log


Fixed
^^^^^

- Lots of game and application compatibility bugs!

  - See GitHub issues and the new Steam AppId support for more details

1.2.0 - 2018-08-18
------------------

Added
^^^^^

- New settings options

  - Overlay position is adjustable
  - Recording output path can be specified
  - Custom user notes for summaries
  - Recording delay setting
  - Recording detail level (simple, normal, verbose)

- Red dot in the overlay to denote recording
- VR support!

  - Overlay is now shown inside the HMD for OpenVR and libOVR supported devices
  - HTC Vive, Oculus devices, Windows Mixed Reality all supported
  - Statistics support for WMR applications via PresentMon
  - Custom ETW logging for SteamVR and Oculus compositor providers
  - New configuration to disable event logging for VR compositors

    - Read `Capture Config <usage.html#capture-config>`_

- Visualisation tool

  - Visualise frame times, reprojections for HMD systems and common session statistics
  - Detailed session visualisation is available using the Select Session tab
  - Visualise multiple session recordings together
  - Save visualised sessions as PDF

- System information

  - Where possible, OCAT now collects detailed system information including
  - Mainboard, OS, CPU, RAM, GPU driver version, number of GPUs
  - Detailed GPU information where possible:
  - AMD: GPU name, core clock, memory clock, memory size
  - Nvidia: GPU name, core clock, memory size
  - Intel: GPU family, core clock, memory size

Changed
^^^^^^^

- OCAT settings are now always visible
- Updated to Vulkan SDK 1.1.82.1
- Updated blacklist
- Update application icon that's more visible on a darker taskbar
- Vulkan overlay now uses an implicit Vulkan layer for the global hook
- Removed support for 32-bit Windows (can still record 32-bit games)

Fixed
^^^^^

- Various game compatibility bugs, see GitHub issues for more details

1.1.0 - 2017-08-09
------------------

Added
^^^^^

- Brand new UI!
- New combined summary data
- Toggle support for the overlay
  
  - Hotkey is P

Changed
^^^^^^^

- Documentation now in Sphinx

  - http://ocat.readthedocs.io/en/latest/

- PresentMon now sourced as a git subtree
- Removed the VS2015 build
- Overlay and PresentMon functionality separated for reliability
- Updated to use Vulkan SDK 1.0.54

Fixed
^^^^^

- Recordings now stop after a detected timeout
- Recording should still work even if the overlay doesn't
  
  - Allows recording even if the overlay won't work
  - Fixes Battlefield 1 and Borderlands 2 among others

1.0.1 - 2017-05-23
------------------

Added
^^^^^

- Continuous integration via AppVeyor
- Redesigned logging and debug system
- Improved documentation on building OCAT from source
- Proper marking of error codes
- Changelogs for GitHub releases!

Changed
^^^^^^^

- Blacklisted UplayWebCore and UbisoftGameLauncher
- Blacklisted Firefox
- Blacklisted RadeonSettings
- Improved DXGI swapchain handling
- Recording hotkey is now F12

Fixed
^^^^^

- Windows 10 Creators Update incompatibility via a PresentMon fix
- Prey on Windows incompatibility
- Doom and The Talos Principle (both Vulkan) incompatibility

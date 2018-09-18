Changelog
=========

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

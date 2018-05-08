Changelog
=========

1.2.0 - 2018-05-08
------------------

Added
^^^^^

- VR support! Overlay within the HMD and recordings for VR-specific ETW counters.
- Visualization tool for the recordings (csv files) with option to save the frame graphs as pdf files.
- Capture config file to individually enable/disable the providers and to set an individual recording detail per provider set.

Changed
^^^^^^^
- Vulkan overlay: Uses implicit layers for the global hook now

Fixed
^^^^^
- Overlay now works also on Vulkan applications using the compute queue for present

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

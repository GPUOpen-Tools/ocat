Usage
=====

To run the program start :program:`OCAT`.

Overlay
---------------

* :guilabel:`Start overlay` enables capturing of all applications. The button turns green if the option is active. That means OCAT will try to inject its overlay into any application that is started afterwards. Notice: For UWP applications, there will be no overlay.
* :guilabel:`Overlay position` Determines the position of the overlay. Choose between top left, top right, bottom left and bottom right. **VR:** The overlay is positioned at the center of the viewspace or the worldspace, depending on the compositor. However, the overlay position still determines the arrangement of the overlay sub-components such as displaying the capture information above or below the FPS/ms data.
* :guilabel:`Overlay visibility hotkey` Hotkey to show and hide the in game overlay globally. Press this button to assign a different hotkey. This setting only works after successful injection. The toggle won't work if no overlay is injected. The default hotkey is :kbd:`P`.
* :guilabel:`Enable overlay when OCAT starts` Activate this option if OCAT should immediately start its global overlay injection on start up.

Capture
---------------
* :guilabel:`Capture hotkey` Hotkey used for starting and stopping a capture. To change the hotkey click the :guilabel:`Capture hotkey` button and press the new hotkey. The default hotkey is :kbd:`F12`.
* :guilabel:`Capture time in seconds` Time period after which a started capture is stopped. If this value is :kbd:`0` the capture will stop only through hotkey or if the captured process stops. The default capture time period is :kbd:`60` seconds.
* :guilabel:`Delayed start in seconds` Determines the delay of the capture start after the capture hotkey is pressed. The default is :kbd:`0` seconds, which means the capture starts immediately after the capture hotkey is pressed.
* :guilabel:`Capture performance for all processes` If enabled pressing the capture hotkey will capture all processes and create a file for each process. If this is disabled only the process of the active window is captured when the hotkey is pressed (the process which is currently in focus). If no active window can be found the capture will default to all processes.
* :guilabel:`Select output folder` Opens a folder dialog to select a folder in which the captures should be saved. The default capture folder is ``Documents\OCAT\Captures``.
* :guilabel:`User note for capture` A user note can be entered here which will be saved in the performance summary file, ``perf_summary.csv``, for each capture.

Launch App
---------------
Allows you to inject the overlay into a single application. Once the application and the command line parameters have been selected, press :guilabel:`Start Application` to run it.

  * :guilabel:`Select target executable` Opens a file dialog to select an executable file that should be started with the OCAT overlay. It is possible to start a UWP app but the overlay will not react to key input. The capture will work correctly.
  * :guilabel:`Commandline arguments` Additional command line arguments to start the executable with.
  * use **steam://run/<AppId>** as a command line argument to prevent the game restarting via the Steam client. Make sure the Steam AppId is the correct one for the selected application and that the Steam client is running.

Note that most games that run via Steam (or other launchers like UPlay) start the respective launcher and terminate afterwards. The launcher will then call the executable which won't be detected by OCAT using this option. You should use the global overlay injection (:guilabel:`Start overlay`) in these cases.

Visualize
---------------
Allows the visualization of the captures for the following metrics: frame times, reprojection times (VR only), and the overall capture statistics for missed frames, average FPS, average frame times, average reprojection times (VR only) and 99th-percentile frame times.

* :guilabel:`Select capture file to visualize` Opens a file dialog to select a capture file whose data should be visualized.
* :guilabel:`Visualize` Opens the visualization window. Multiple visualization windows can be open simultaneously.


Visualization Window
--------------------
Displays the frame graphs of the loaded captures.

**Home**

* :guilabel:`Select capture file to add` Opens a file dialog to select a capture file whose data should be visualized within the visualization window. Multiple captures can be visualized in the same window to allow direct capture-to-capture comparisons.
* :guilabel:`Load` Loads the selected capture file, whose data is visualized on top of the current loaded captures.
* :guilabel:`Select capture` Opens a list of the loaded captures. Click on a capture for selection.
* :guilabel:`Remove capture file` Removes the selected capture from the visualization window.
* :guilabel:`Save graph` Saves the current frame graph as a PDF file.
* :guilabel:`Show frame analysis` Shows per frame data based on the rendering time of the application and of the compositor (VR only) of the selected capture. Frames are displayed in chunks of about 500 frames, and stepping through the chunks is possible via the arrows on the top right of the frame graph.
* :guilabel:`Frame times` Shows a frame graph of the frame times of the loaded captures.
* :guilabel:`Reprojections` Shows a frame graph of the reprojection times of the loaded captures (VR only).
* :guilabel:`Capture statistics` Shows overall capture statistics of the loaded captures. Switch between the metrics using the arrows on the top right of the graph. The following metrics can be displayed:
 - Missed frames
 - Average FPS
 - Average frame times
 - Average reprojection times (VR only)
 - 99th-percentile frame times


**Controls**

Displays the controls to navigate within the frame graph window.

General options
---------------


Capture
---------------

Capture starts after the hotkey button is pressed and ends with another hotkey press or if the capture time is reached. If a capture is in progress this will be displayed above the program version in the OCAT configuration and the overlay, if enabled, shows a red dot.

* If no capture is in progress the hotkey for starting a capture is shown (default: ``F12``) above the program version in the OCAT configuration.
* Which processes are captured depends on the :guilabel:`Capture performance for all processes` option.

Capture files
----------

Captures are saved by default in the ``Documents\OCAT\Captures`` folder. The output folder can be changed via the ``Select output folder`` option under the Capture tab.
A detailed ``.csv`` file is created for each capture per supported provider set. Following provider sets are supported:

* ``DXGI`` for desktop applications
* ``SteamVR`` for VR games based on the openvr SDK compositor
* ``OculusVR`` for VR games based on the LibOVR SDK compositor
* ``WMR`` for Windows Mixed Reality VR games based on the DWM compositor

A summary for each capture can be found in the ``perf_summary.csv`` file.  

An empty capture file can be caused by disabling the :guilabel:`Capture performance for all processes` option and focusing a different process when pressing the capture hotkey.

Capture config
--------------
The capture config file ``captureConfig.json`` can be found in ``Documents\OCAT\Config``.
The ETW provider sets can be individually enabled and disabled, further individual capture details can be provided for each set.
If the :kbd:`Default` or an invalid capture detail is specified, the capture detail falls back to :kbd:`Verbose`.
If an entry for a provider set is missing, it is by default enabled and uses the :kbd:`Verbose` capture detail.
On the first run, OCAT will generate a ``captureConfig.json`` file. To restore the default settings, delete the capture config file. A new one will be generated on the next run.
The capture detail options are :kbd:`Simple`, :kbd:`Normal` and :kbd:`Verbose`.

Blacklist
---------

Applications can be excluded from DLL-Injection through blacklisting based on the executable name. The blacklists, a default and a user blacklist, can be found in ``Documents\OCAT\Config``.
All processes on the blacklists are not showing the overlay and no captures are created. On the first run, OCAT will generate or update the default blacklist and generate a dummy user blacklist.
The user should add executables to the user blacklist to make sure they won't get overwritten when OCAT is updated. Each line must contain one executable name (case insensitive).

Logs
---------

Logs are saved in ``Documents\OCAT\Logs``. The logs include:

* ``PresentMonLog`` containing information about the capturing and start of processes
* ``GlobalHook32Log`` and ``GlobalHook64Log`` information about the state of the global hook processes
* ``GameOverlayLog`` information about all injected DLLs

Known Issues
------------

* Windows 7: PresentMon is not creating captures.
* UWP: Global hooking for overlay does not work.
* UPlay: The overlay does not work with UPlay games due to security mechanisms. This applies both for the global and the explicit hook. The capture function works fine.
* Steam: DOOM, Wolfenstein 2: The New Colossus, Rise of the Tomb Raider (and many others): The explicit hook for the overlay will only work if **steam://run/<AppId>** is parsed as commandline argument, to prevent a relaunch of the game by Steam. The global hook and capture function work normally.
* Final Fantasy XV: the global hook does not work, use the explicit hook with **steam://run/<AppId>**.
* Frostpunk: the global hook does not work, use the explicit hook
* SteamVR Battlezone: HMD overlay does not work with the global hook. Use the explicit hook for enabling the overlay within the HMD.

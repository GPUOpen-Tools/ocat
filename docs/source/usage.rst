Usage
=====

To run the program start :program:`OCAT`.

Capture
---------------

* :guilabel:`Start Overlay` enables capturing of all applications. The button turns green if the option is active. That means OCAT will try to inject its overlay into any application that is started afterwards. Notice: For UWP applications, there will be no overlay, but the recording will still work when pressing the hotkey.
* The setting :guilabel:`Inject overlay on start` determines whether :guilabel:`Start Overlay` is active directly after OCAT started.


Advanced
---------------
Allows to capture a single application. Once the application and the command line parameters have been selected, press :guilabel:`Start Application` to run it.

  * :guilabel:`Select target executable` Opens a file dialog to select an executable file that should be started for recording. It is possible to start a UWP app but the GameOverlay will not react to key input. The recording will work correctly.
  * :guilabel:`Commandline Arguments` Additional command line arguments to start the executable with.

Note that most games that run in Steam (or UPlay) start the respective launcher and terminate afterwards. The launcher will then call the executable which won't be detected by OCAT using this option. You should use the global overlay injection (:guilabel:`Start Overlay`) in these cases.


Settings
-----------------

* :guilabel:`Overlay position` Determines the position of the overlay. Choose between top left, top right, bottom left and bottom right. **VR:** The overlay is positioned at the center of the viewspace or the worldspace, depending on the compositor. However, the overlay position still determines the arrangement of the overlay sub-components such as displaying the capture information above or below the FPS/ms data.
* :guilabel:`Overlay hotkey` Hotkey to show and hide the in game overlay globally. Press this button to assign a different hotkey. This setting only works after successful injection. The toggle won't work, if no overlay is injected. The default hotkey is :kbd:`P`.
* :guilabel:`Inject overlay on start` Activate this option, if OCAT should immediately start global overlay injection on start up.
* :guilabel:`Record performance for all processes` If enabled pressing the recording hotkey will record all processes and create a file for each process. If this is disabled only the process of the active window is recorded when the hotkey is pressed (the process which is currently in focus). If no active window can be found the recording will default to all processes.
* :guilabel:`Recording hotkey` Hotkey used for starting and stopping a recording. To change the hotkey click the :guilabel:`Recording hotkey` button and press the new hotkey. The default hotkey is :kbd:`F12`.
* :guilabel:`Recording time period in seconds` Time period after which a started recording is stopped. If this value is zero the recording will stop only through hotkey or if the recorded process stops. The default recording time period is :kbd:`60` seconds.
* :guilabel:`Recording delay in seconds` Determines the delay of the recording start after the recording hotkey is pressed. The default is :kbd:`0` seconds, which means the recording starts immediately after the recording hotkey is pressed.
* :guilabel:`Recording detail` Determines the level of detail of the recording. The options are :kbd:`Simple`, :kbd:`Normal` and :kbd:`Verbose`.


Visualization
---------------
Allows the visualization of the recordings for the following metrics: frame times, reprojection times (VR only), and the overall session satistics missed frames, average FPS, average frame times, average reprojection times (VR only) and 99th-percentile frame times.

* :guilabel:`Select recording file` Opens a file dialog to select a recording file whose data should be visualized.
* :guilabel:`Visualize` Opens the visualization window. Multiple visualization windows can be open simultaneously.


Visualization Window
--------------------
Displays the frame graphs of the loaded sessions.

**Home**

* :guilabel:`Select recording file to add` Opens a file dialog to select a recording file whose data should be visualized within the visualization window. Multiple sessions can be visualized in the same window to allow direct session-to-session comparisons.
* :guilabel:`Load` Loads the selected recording file, whose data is visualized on top of the current sessions.
* :guilabel:`Select Session` Opens a list of the loaded sessions. Click on a session for selection.
* :guilabel:`Remove session` Removes the selected session from the visualization window.
* :guilabel:`Save Graph` Saves the current frame graph as .pdf file.
* :guilabel:`Show frame analysis` Shows per frame data based on the rendering time of the application and of the compositor (VR only) of the selected session. Frames are displayed in chunks of about 500 frames, stepping through the chunks is possible via the arrows on the top right of the frame graph.
* :guilabel:`Frame times` Shows a frame graph of the frame times of the loaded sessions.
* :guilabel:`Reprojection times` Shows a frame graph of the reprojection times of the loaded sessions (VR only).
* :guilabel:`Session statistics` Shows overall session statistics of the loaded sessions. Switch between the metrics using the arrows on the top right of the graph. Following metrics can be displayed:
 - Missed frames
 - Average FPS
 - Average frame times
 - Average reprojection times (VR only)
 - 99th-percentile frame times


**Controls**

Displays the controls to navigate within the frame graph window.

General options
---------------


Recording
---------------

Recording starts after the hotkey button is pressed and ends with another hotkey press or if the recording time is reached. If a recording is in progress this will be displayed above the program version in the OCAT configuration.  

* If no recording is in progress the hotkey for starting a recording is shown (default: ``F12``).
* Which processes are recorded depends on the :guilabel:`Record performance for all processes` option.

Recordings
----------

Recordings are saved in the ``Documents\OCAT\Recordings`` folder. A detailed ``.csv`` file is created for each recording per supported provider set. Following provider sets are supported:

* ``DXGI`` for desktop applications
* ``SteamVR`` for VR games based on the openvr SDK compositor
* ``OculusVR`` for VR games based on the LibOVR SDK compositor
* ``WMR`` for Windows Mixed Reality VR games based on the DWM compositor

A summary for each recording can be found in the ``perf_summary.csv`` file.  

An empty recording file can be caused by disabling the :guilabel:`Record performance for all processes` option and focusing a different process when pressing the recording hotkey.
Also games that use special characters (like ``Ghost Recon® Wildlands``) won't show their executable names in the recording files.

Capture Config
--------------
The capture config .json file can be found in ``Documents\OCAT\Config``. The ETW provider sets can be individually enabled and disabled, further individual recording details can be provided for each set. If the :kbd:`Default` recording detail is specified, the recording detail of the global :guilabel:`Recording Detail` setting is used. If an entry for a provider set is missing, it is by default enabled and uses the global recording detail.
On the first run, OCAT will generate a ``capture-config.json`` file. To restore the default settings, delete the capture config file. A new one will be generated on the next run.

Blacklist
---------

Applications can be excluded from DLL-Injection through blacklisting based on the executable name. The blacklist can be found in ``Documents\OCAT\Config``. 
All processes on the black list are not showing the overlay. On the first run, OCAT will generate a default blacklist. Each line must contain one executable name (case insensitive).

Logs
---------

Logs are saved in ``Documents\OCAT\Logs``. The logs include:

* ``PresentMonLog`` containing information about the capturing and start of processes
* ``GlobalHook32Log`` and ``GlobalHook64Log`` information about the state of the global hook processes
* ``GameOverlayLog`` information about all injected dlls

Known Issues
------------

* Windows 7: PresentMon is not creating recordings.
* UWP: Global hooking for overlay is not working.
* UPlay: The overlay does not work with UPlay games due to security mechanisms. This applies both for the global and the explicit hook. Recording function works fine.
* Steam: DOOM and Wolfenstein: The explicit hook for the overlay does not work due to a relaunch of the game by Steam. Global hook and recording function works.
* SteamVR Battlezone: HMD overlay does not work with the global hook. Use the explicit hook for enabling the overlay within the HMD.

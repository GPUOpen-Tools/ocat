Usage
=====

To run the program start :program:`OCAT`.

Capture
---------------

* :guilabel:`Enable Overlay` enables capturing of all applications. The button turns green if the option is active. That means OCAT will try to inject its overlay into any application that is started afterwards. Notice: For UWP applications, there will be no overlay, but the recording will still work when pressing the hotkey.
* The setting :guilabel:`Overlay injection on start` determines whether :guilabel:`Enable Overlay` is active directly after OCAT started.


Advanced
---------------
Allows to capture a single application. Once the application and the command line parameters have been selected, press :guilabel:`Start Application` to run it.

  * :guilabel:`Select target executable` Open Executable to select a file that should be started for recording. It is possible to start a UWP app but the GameOverlay will not react to key input. The recording will work correctly.
  * :guilabel:`Commandline Arguments` Additional command line arguments to start the executable with.

Note that most games that run in Steam (or UPlay) start the respective launcher and terminate afterwards. The launcher will then call the executable which won't be detected by OCAT using this option. You should use the global overlay injection (:guilabel:`Enable Overlay`) in these cases.


Settings
-----------------

* :guilabel:`Overlay hotkey` Hotkey to show and hide the in game overlay globally. Press this button to assign a different hotkey. This setting only works after successful injection. The toggle won't work, if no overlay is injected.
* :guilabel:`Recording hotkey` Hotkey used for starting and stopping a recording. To change a hotkey click the :guilabel:`Recording hotkey` button and press the new hotkey. The default hotkey is :kbd:`F12`.
* :guilabel:`Recording time period in seconds` Time period after which a started recording is stopped. If this value is zero the recording will stop only through hotkey or if the recorded process stops.
* :guilabel:`Record performance for all processes` If :guilabel:`Record performance for all processes` is enabled pressing the recording hotkey will record all processes and create a file for each process. If this is disabled only the process of the active window is recorded when the hotkey is pressed (the process which is currently in focus). If no active window can be found the recording will default to all processes.
* :guilabel:`Overlay injection on start` Activate this option, if OCAT should immediately start global overlay injection on start up.

General options
---------------


Recording
---------------

Recording starts after the hotkey button is pressed and ends with another hotkey press or if the recording time is reached. If a recording is in progress this will be displayed above the program version in the OCAT configuration.  

* If no recording is in progress the hotkey for starting a recording is shown (default: ``F12``).
* Which processes are recorded depends on the :guilabel:`Record performance for all processes` option.

Recordings
----------

Recordings are saved in the ``Documents\OCAT\Recordings`` folder. A detailed ``.csv`` file is created for each recording. A summary for each recording can be found in the ``perf_summary.csv`` file.  

An empty recording file can be caused by disabling the :guilabel:`Record performance for all processes` option and focusing a different process when pressing the recording hotkey.
Also games that use special characters (like ``Ghost Recon® Wildlands``) won't show their executable names in the recording files.

Blacklist
---------

Applications can be excluded from DLL-Injection through blacklisting based on the executable name. The blacklist can be found in ``Documents\OCAT\Config``. 
All processes on the black list are not showing the overlay. On the first run, OCAT will generate the blacklist. Each line must contain one executable name (case insensitive).

Logs
---------

Logs are saved in ``Documents\OCAT\Config``. The logs include:

* ``PresentMonLog`` containing information about the capturing and start of processes
* ``GlobalHook32Log`` and ``GlobalHook64Log`` information about the state of the global hook processes
* ``GameOverlayLog`` information about all injected dlls

Known Issues
------------

* Windows 7: PresentMon is not creating recordings.
* UWP: Global hooking for overlay is not working.

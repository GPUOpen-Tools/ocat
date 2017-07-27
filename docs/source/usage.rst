Usage
=====

To run the program start :program:`OCAT`.

Recording options
-----------------

* :guilabel:`Recording Hotkey` hotkey used for starting and stopping of recording. To change a hotkey click the Recording Hotkey button and press the new hotkey. The default hotkey is :kbd:`F11`.
* :guilabel:`Recording Time Period in Seconds` time period after which a started recording is stopped. If this value is zero the recording will stop only through hotkey or if the recorded process stops.


Capture options
---------------

* :guilabel:`Capture All Applications` enables capturing of all applications after Start was pressed. Notice: For UWP applications, there will be no overlay, but the recording will still work when pressing the hotkey.
* :guilabel:`Capture Single Application` allows to capture a single application. Once the application and the command line parameters have been selected, press Start to run it.

  * :guilabel:`Select target executable` Open Executable to select a file that should be started for recording. It is possible to start a UWP app but the GameOverlay will not react to key input. The recording will work correctly.
  * :guilabel:`Commandline Arguments` Additional command line arguments to start the executable with.

General options
---------------

* :guilabel:`Simple recording` appends the application name, date and time, average fps, average ms per frame and the 99th percentile ms per frame to the ``perf_summary.csv`` file
* :guilabel:`Save detailed recordings` saves detailed information for one process in a ``perf_< process name >_< date and time >.csv``.
* :guilabel:`Record performance for all processes` If this is enabled pressing the recording hotkey will record all processes and create a file for each process. If this is disabled only the process of the active window is recorded when the hotkey is pressed (this is, the process which is currently in focus).

Start Recording
---------------

Recording starts after the hotkey button is pressed and ends with another hotkey press or if the recording time is reached. If a recording is in progress this will be displayed above the program version in the OCAT configuration.  

If no recording is in progress the hotkey for starting a recording is shown.

* If :guilabel:`Capture All Applications` is selected a recording can be started by pressing the hotkey. If changes are made the recording has to be stopped and restarted with the new options.  
* If :guilabel:`Capture Single Application` is selected capturing can only be started after a target executable was selected.
* Which processes are recorded depends on the :guilabel:`Record Foreground Window` option.

Recordings
----------

Recordings are saved in the ``Documents\OCAT\Recordings`` folder. Depending on the :guilabel:`General options` a detailed ``.csv`` file is created for each recording. A summary for each recording can be found in the ``perf_summary.csv`` file if :guilabel:`Simple recording` is enabled.  

An empty recording file can be caused by disabling the :guilabel:`Record performance for all processes` option and focusing a different process when pressing the recording hotkey.

Blacklist
---------

Applications can be excluded from capturing through blacklisting based on the executable name. The blacklist can be found in ``Documents\OCAT\Config``. All processes on the black list are not recorded. On the first run, OCAT will generate the blacklist. Each line must contain one executable name.

Logs
---- 

Logs are saved in ``Documents\OCAT\Config``. The logs include:

* ``PresentMonLog`` containing information about the capturing and start of processes
* ``GlobalHook32Log`` and ``GlobalHook64Log`` information about the state of the global hook processes
* ``GameOverlayLog`` information about all injected dlls with the format < GameOverlay(32|64) > < injected process id > < log message >

Notes
-----

* :guilabel:`Capturing STEAM` If Steam is already running :guilabel:`Capture All Applications` has to be used for the overlay to work.
* :guilabel:`Disable overlay` The overlay can be disabled by setting a global environment variable with name ``OCAT_OVERLAY_ENABLED`` and value ``0``. Global environment variables can be set in System->Advanced system settings->Environment Variables.

Known Issues
------------

* Windows 7: PresentMon is not creating recordings.
* UWP: Global hooking for overlay is not working.

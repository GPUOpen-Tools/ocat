---
title: Readme
---

# OCAT

## Usage
To run the program start __OCAT__  

### Recording options:
* __Recording Hotkey:__ hotkey used for starting and stopping of recording. To change
a hotkey click the Recording Hotkey button and press the new hotkey. The default hotkey is __F11__.
* __Recording Time Period in Seconds:__ time period after which a started recording is stopped. If this value is zero the recording will stop only through hotkey or if the recorded process stops.

### Capture options:
* __Capture All Applications:__ enables capturing of all applications after Start was pressed. Notice: For UWP applications, there will be no overlay, but the recording will still work when pressing the hotkey.
* __Capture Single Application:__ allows to capture a single application. Once the application and the command line parameters have been selected, press Start to run it.
  * __Select target executable:__ Open Executable to select a file that should be started for recording. It is possible to start a UWP app but the gameoverlay will not react to key input. The recording will work correctly.
  * __Commandline Arguments:__ Additional command line arguments to start the executable with.

### General options:
* __Simple recording:__ appends the application name, date and time, average fps, average ms per frame and the 99th percentile ms per frame to the `perf_summary.csv` file
* __Save detailed recordings:__ saves detailed information for one process in a
`perf_< process name >_< date and time >.csv`.
* __Record performance for all processes:__ If this is enabled pressing the recording hotkey will record all processes and create a file for each process. If this is disabled only the process of the active window is recorded when the hotkey is pressed (this is, the process which is currently in focus).

### Start Recording:  
Recording starts after the hotkey button is pressed and ends with another hotkey press or if the recording time is reached. If a recording is in progress this will be displayed above the program version in the OCAT configuration.  

If no recording is in progress the hotkey for starting a recording is shown.
* If __Capture All Applications__ is selected a recording can be started by pressing the hotkey. If changes are made the recording has to be stopped and restarted with the new options.  
* If __Capture Single Application__ is selected capturing can only be started after a target executable was selected.
* Which processes are recorded depends on the __Record Foreground Window__ option.

### Recordings:
Recordings are saved in the `Documents\OCAT\Recordings` folder. Depending on the __General options__ a detailed `.csv` file is created for each recording. A summary for each recording can be found in the `perf_summary.csv` file if __Simple recording__ is enabled.  

An empty recording file can be caused by disabling the __Record performance for all processes__ option and focusing a different process when pressing the recording hotkey.

### Blacklist
Applications can be exluded from capturing through blacklisting based on the executable name. The blacklist can be found in `Documents\OCAT\Config`. All processes on the black list are not recorded. On the first run, OCAT will generate the blacklist. Each line must contain one executable name.

### Logs
Logs are saved in `Documents\OCAT\Config`. The logs include:
* `PresentMonLog` containing information about the capturing and start of processes
* `GlobalHook32Log` and `GlobalHook64Log` information about the state of the global hook processes
* `gameoverlayLog` information about all injected dlls with the format < gameoverlay(32|64) > < injected process id > < log message >

### Notes

* __Capturing STEAM:__ If Steam is already running __Capture All Applications__ has to be used for the overlay to work.
* __Disable overlay:__ The overlay can be disabled by setting a global environment variable with name __OCAT_OVERLAY_ENABLED__ and value __0__. Global environment variables can be set in System->Advanced system settings->Environment Variables.

### Known Issues

* Windows 7: PresentMon is not creating recordings.
* UWP: Global hooking for overlay is not working.

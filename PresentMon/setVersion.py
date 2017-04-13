#!/usr/bin/env python3
import os
import re
import sys

if len(sys.argv) != 2:
    sys.exit("specify the new file version in the format 0.0.0\n")
else:
    versionL = str(sys.argv[1])
    if len(versionL) != 5:
        sys.exit("invalid file version size")

    versionU = versionL.replace('.',',')

    resourceFiles = [
        "gameoverlay\\vulkan\\VK_LAYER_OCAT_overlay.rc",
        "gameoverlay\\d3d\\res\\resource.rc",
        "PresentMon\\PresentMon.rc",
        "UWPDebug\\UWPDebug.rc",
        "Wrapper\\app.rc",
        "DLLInjector\\DLLInjector.rc",
        "GlobalHook\\GlobalHook.rc",
		"UWPOverlay\\UWPOverlay.rc"]

    regexFileVersionU = re.compile('(?<=FILEVERSION )\d,\d,\d')
    regexFileVersionL = re.compile('(?<="FileVersion", ")\d.\d.\d')
    regexProductVersionU = re.compile('(?<=PRODUCTVERSION )\d,\d,\d')
    regexProductVersionL = re.compile('(?<="ProductVersion", ")\d.\d.\d')

    for fileName in resourceFiles:
        filePath = os.path.abspath(fileName)
        print(filePath)

        lines = []
        with open(filePath, 'r', encoding='utf-16') as infile:
            for line in infile:
                line = regexFileVersionU.sub(versionU, line)
                line = regexFileVersionL.sub(versionL, line)
                line = regexProductVersionU.sub(versionU, line)
                line = regexProductVersionL.sub(versionL, line)
                lines.append(line)
        with open(filePath, 'w', encoding='utf-16') as outfile:
            for line in lines:
                outfile.write(line)

    frontendFile = os.path.abspath("Frontend\\Properties\\AssemblyInfo.cs")
    print(frontendFile)

    lines = []
    regexAssemblyVersion = re.compile('(?<=AssemblyVersion\(")\d.\d.\d')
    regexAssemblyFileVersion = re.compile('(?<=AssemblyFileVersion\(")\d.\d.\d')
    with open(frontendFile, 'r') as infile:
        for line in infile:
            line = regexAssemblyVersion.sub(versionL, line)
            line = regexAssemblyFileVersion.sub(versionL, line)
            lines.append(line)
    with open(frontendFile, 'w') as outfile:
        for line in lines:
            outfile.write(line)

#!/usr/bin/bash
sed -i "37s/([^()]*)/\(\"$(git describe --long)\"\)/g" $(pwd)/../../../Properties/AssemblyInfo.cs
#sleep 1 # Use for debugging bash script

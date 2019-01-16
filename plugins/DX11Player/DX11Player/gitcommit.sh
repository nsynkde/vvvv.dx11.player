#!/usr/bin/bash
sed -i "37s/([^()]*)/\(\"$(git describe --long)\"\)/g" $(pwd)/../../properties/AssemblyInfo.cs
#sleep 10 # Use for debugging bash script

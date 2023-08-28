#!/bin/bash

#Prerequisite:
#- download innosetup from https://jrsoftware.org/isinfo.php
#- install c++ build tools from https://visualstudio.microsoft.com/visual-cpp-build-tools/, select :
#    - C++ CMake tools for Windows (it will select MSVC 2022)
#    - Testing tools core features
#    - Windows 10 SDK

win10_sdk_path="/c/Program Files (x86)/Windows Kits/10/bin/10.0.20348.0/x64"
innosetup_path="/c/Program Files (x86)/Inno Setup 6"
iss_file="installer.iss"

echo ""
echo "Moolticute building and signing script"

if ! [ -d "$win10_sdk_path" ]; then
    echo "Folder $win10_sdk_path does not exist."
	echo "Have you installed windows 10 sdk?"
    read -p "Press Enter to continue..."
    exit 1
fi

if ! [ -d "$innosetup_path" ]; then
    echo "Folder $innosetup_path does not exist."
	echo "Have you installed innosetup?"
    read -p "Press Enter to continue..."
    exit 1
fi

# Prompt the user to select betas or release
read -p "1 for betas, 2 for releases: " choice

# Moolticute beta latest file url contents
if [ "$choice" == "1" ]; then
	update_file_content=$(curl -s "https://betas.themooltipass.com/updater.json")
	
	# Extract browser_download_url for the first asset
	exe_dl_url=$(echo "$update_file_content" | grep 'browser_download_url' | grep 'exe' | cut -d'"' -f4)

	# Check for .exe
	if ! [[ $exe_dl_url == *".exe"* ]]; then
		echo "$update_file_content"
		echo "Update file contents invalid"
		read -p "Press Enter to continue..."
		exit 1
	fi
	
	# Convert .exe to .zip url
	zip_dl_url="${exe_dl_url//.exe/.zip}"
	zip_dl_url="${zip_dl_url//moolticute_setup/moolticute_portable_win32}"
elif [ "$choice" == "2" ]; then
	release_file_content=$(curl -s -L "https://api.github.com/repos/mooltipass/moolticute/releases/latest")
	
	# Extract browser_download_url for the zip asset
	zip_dl_url=$(echo "$release_file_content" | grep 'browser_download_url' | grep 'zip' | cut -d'"' -f4)	
else
    echo "Invalid choice. Please enter 1 or 2."
    read -p "Press Enter to continue..."
    exit 1
fi

# Download zip file
echo "Downloading Moolticute zip file..."
curl -O -L -s "$zip_dl_url"

echo "Downloading installer script..."
curl -O -L -s https://raw.githubusercontent.com/mooltipass/moolticute/master/win/installer.iss

echo "Checking for downloaded zipped archive..."

# Find zip files in the directory
zip_file_path=$(find "." -type f -name "*.zip")

# Check if exactly one zip file was found
if ! [ $(echo "$zip_file_path" | wc -l) -eq 1 ]; then
    echo "Either no zip files found or more than one zip file found in the directory."
    read -p "Press Enter to continue..."
    exit 1
fi

# Extract the filename from the path
zip_filename=$(basename "$zip_file_path")

# Extract text between prefixes and suffixes
prefix="moolticute_portable_win32_"
suffix=".zip"

# Remove the prefix from the beginning of the string
temp_string="${zip_filename#$prefix}"

# Remove the suffix from the end of the string
moolticute_version="${temp_string%$suffix}"
echo "Detected Moolticute version: $moolticute_version"

# unzip file
echo "Unzipping downloaded file..."
unzip -q -o "$zip_filename"

# Find folders in the directory
folders=($(find "." -mindepth 1 -maxdepth 1 -type d))

# Filter out the current directory (.)
filtered_folders=()
for folder in "${folders[@]}"; do
    if [ "$folder" != "." ] && [ "$folder" != "./build" ]; then
        filtered_folders+=("$folder")
    fi
done

# Check the number of folders
num_folders=${#filtered_folders[@]}

if ! [ "$num_folders" -eq 1 ]; then
    echo "Either no folders found or more than one found in the directory."
    read -p "Press Enter to continue..."
    exit 1
fi
	
# Extract the folder name
folder_name=$(basename "${filtered_folders[0]}")

# Change iss contents
echo "Replacing ISS file contents..."
sed -i 's/^; #define MyAppVersion "1.0"/#define MyAppVersion "'"$moolticute_version"'"/' "$iss_file"
sed -i 's#C:\\moolticute_build#'"$folder_name"'#' "$iss_file"

# Expand path
export PATH=$PATH:"$win10_sdk_path"
export PATH=$PATH:"$innosetup_path"

# Sign files
echo "Signing dlls and exes... feel free to take a coffee"
signtool sign //fd SHA256 //td SHA256 //tr http://time.certum.pl $(find . -type f \( -name "*.dll" -o -name "*.exe" \))

# Copy required images
cp "$innosetup_path\WizClassicSmallImage-IS.bmp" WizModernSmallImage-IS.bmp
cp "$innosetup_path\SetupClassicIcon.ico" Setup.ico

# Launch build
echo "Launching installer build..."
iscc "$iss_file"

# Sign installer
echo "Signing installer..."
signtool sign //fd SHA256 //td SHA256 //tr http://time.certum.pl  $(find build -type f \( -name "*.dll" -o -name "*.exe" \))
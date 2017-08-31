param([string]$configuration,[string]$platform,[string]$suffix)
$files = @()

if (!$PSBoundParameters.ContainsKey('configuration'))
{
	Write-Host 'configuration not specified!'
}
if (!$PSBoundParameters.ContainsKey('platform'))
{
	Write-Host 'platform not specified!'
}
if (!$PSBoundParameters.ContainsKey('suffix'))
{
	Write-Host 'suffix not specified!'
}

#
Write-Host "Creating distribution for"
Write-Host "- configuration = " $configuration
Write-Host "- platform      = " $platform
Write-Host "- suffix        = " $suffix

# (1) Copy files to distribution directory.

# (1.1) Add files of Egoboo.
$files += ".\product\game\${configuration}\${platform}\game.exe"

# (1.2) Add the dependencies of external dependencies.
cd ".\external"; (./distribute.ps1 -configuration ${configuration} -platform ${platform} -suffix ${suffix}); cd ..
New-Item -Force   -ItemType Directory ".\distribution\${configuration}\${platform}"
Copy-Item -Path ".\external\egoboo-external-$($configuration.ToLower())-${platform}-${suffix}.zip" .\distribution\${configuration}\${platform}
Expand-Archive ".\distribution\${configuration}\${platform}\egoboo-external-$($configuration.ToLower())-${platform}-${suffix}.zip" -DestinationPath ".\distribution\${configuration}\${platform}"
Remove-Item ".\distribution\${configuration}\${platform}\egoboo-external-$($configuration.ToLower())-${platform}-${suffix}.zip"
# (1.5) Do copy files to distribution directory.
foreach ($file in $files) {
	New-Item -Force .\distribution\${configuration}\${platform} -ItemType Directory
	Copy-Item -Path ${file} -Destination .\distribution\${configuration}\${platform}
}

# (2) Create a zip file with the contents of the distribution directory.
$zipFile = "egoboo-$($configuration.ToLower())-${platform}-${suffix}.zip"
# (2.1) Delete any existing zip file of the same name.
If (Test-Path ${zipFile}){
	Remove-Item ${zipFile}
}
# (2.2) Create the zip file.
Compress-Archive -Path .\distribution\${configuration}\${platform}* -DestinationPath ${zipFile}

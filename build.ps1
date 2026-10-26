param(
	[switch]$KeepVersion,
	[Parameter(ValueFromRemainingArguments = $true)]
	[string[]]$BuildArgs
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectDir = Join-Path $repoRoot "project"
$msxglBuild = Join-Path $repoRoot "MSXgl\engine\script\js\build.js"
$versionFile = Join-Path $projectDir "version.h"

if (-not (Test-Path -LiteralPath $msxglBuild -PathType Leaf)) {
	throw "MSXgl build script not found: $msxglBuild"
}

$nodeCommand = Get-Command node.exe -ErrorAction SilentlyContinue
if (-not $nodeCommand) {
	throw "Node.js was not found. Install Node.js and make node.exe available in PATH."
}

Push-Location $projectDir
try {
	& $nodeCommand.Source $msxglBuild @BuildArgs
	$buildExitCode = $LASTEXITCODE
}
finally {
	Pop-Location
}


if (($buildExitCode -eq 0) -and (-not $KeepVersion) -and ($BuildArgs -notcontains "clean"))
{
	$versionContent = Get-Content -LiteralPath $versionFile -Raw
	$minorPattern = '(?m)^#define APP_VERSION_MINOR (\d+)\r?$'
	$minorMatch = [regex]::Match($versionContent, $minorPattern)
	if (-not $minorMatch.Success)
	{
		throw "Could not read APP_VERSION_MINOR from $versionFile"
	}

	$nextMinor = [int]$minorMatch.Groups[1].Value + 1
	$versionContent = [regex]::Replace($versionContent, $minorPattern, "#define APP_VERSION_MINOR $nextMinor", 1)
	Set-Content -LiteralPath $versionFile -Value $versionContent -NoNewline -Encoding ascii
	Write-Host "Next build minor version: $nextMinor"
}
exit $buildExitCode

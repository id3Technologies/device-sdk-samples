param($language = "all")

$cmake = "C:\Program Files\CMake\bin\cmake"
$msbuild = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\MSBuild\\Current\\Bin\\msbuild.exe"

function BuildCpp {
    param (
        [bool]$NoCallback = $false,  # Valeur par défaut
        [bool]$LoopModeThread = $false  # Valeur par défaut
    )
    try
    {
        $NoCallbackStr = if ($NoCallback) { "ON" } else { "OFF" }
        $LoopModeThreadStr = if ($LoopModeThread) { "ON" } else { "OFF" }
        Write-Output "==============================================================================="
        Write-Output "Build cpp CaptureSampleCLI (NO_CALLBACK $NoCallbackStr, LOOP_MODE_THREAD $LoopModeThreadStr)"
        Write-Output "==============================================================================="
        # c/c++ build
        Push-Location
        Set-Location cpp/CaptureSampleCLI
        mkdir -Force build
        Set-Location build
        Remove-Item -Recurse -Force *
        # build x64
        & "C:\Program Files\CMake\bin\cmake" -G "Visual Studio 17 2022" -DLOOP_MODE_THREAD="$LoopModeThreadStr" -DNO_CALLBACK="$NoCallbackStr" ..
        & "C:\Program Files\CMake\bin\cmake" --build . --config Release
        if (-not $?)
        {
            throw
        }
        Set-Location Release
        .\CaptureSampleCLI.exe
        if ($LastExitCode -ne 0)
        {
            throw
        }
    }
    catch
    {
        Write-Output "Error in cpp x64"
        throw
    }
    finally
    {
        Pop-Location
    }
}

function BuildCppOpenCv {
    param (
        [bool]$PlugAndPlay = $false  # Valeur par défaut
    )
    try
    {
        $PlugAndPlayStr = if ($PlugAndPlay) { "ON" } else { "OFF" }
        Write-Output "==============================================================================="
        Write-Output "Build cpp CaptureSampleOpenCv (PLUG_AND_PLAY $PlugAndPlayStr)"
        Write-Output "==============================================================================="
        # c/c++ build
        Push-Location
        Set-Location cpp/CaptureSampleOpenCv
        mkdir -Force build
        Set-Location build
        Remove-Item -Recurse -Force *
        # build x64
        & "C:\Program Files\CMake\bin\cmake" -G "Visual Studio 17 2022" -DPLUG_AND_PLAY="$PlugAndPlayStr" ..
        & "C:\Program Files\CMake\bin\cmake" --build . --config Debug
        if (-not $?)
        {
            throw
        }
    }
    catch
    {
        Write-Output "Error in cpp x64"
        throw
    }
    finally
    {
        Pop-Location
    }
}

function BuildCppAll {
    BuildCpp $false $false
    BuildCpp $false $true
    BuildCpp $true  $false
    BuildCpp $true  $true
    BuildCppOpenCv $false
    BuildCppOpenCv $true
}

function BuildCppMFC {
    try
    {
        Write-Output "==============================================================================="
        Write-Output "Build cpp MFC"
        Write-Output "==============================================================================="
        # c/c++ build
        Push-Location
        Set-Location cpp/MFC
        & $msbuild id3DevicesSamples.sln /t:Build /p:Configuration=Release
        if (-not $?)
        {
            throw
        }
    }
    catch
    {
        Write-Output "Error in cpp x64"
        throw
    }
    finally
    {
        Pop-Location
    }
}

function BuildDotnet {
    try
    {
        Write-Output "==============================================================================="
        Write-Output "Build dotnet"
        Write-Output "==============================================================================="
        Push-Location
        Set-Location dotnet
        #& "C:\\nuget\\nuget" restore id3.BioSeal.Samples.sln
        & $msbuild id3.Devices.Samples.sln /t:Build /p:Configuration=Release
        if (-not $?) 
        {
            throw
        }
    }
    catch
    {
        Write-Output "Error in dotnet"
        throw
    }
    finally
    {
        Pop-Location
    }
}

function BuildPython {
    try
    {
        Write-Output "==============================================================================="
        Write-Output "Build python"
        Write-Output "==============================================================================="
        Push-Location
        Set-Location python
        python -m venv sample-env
        sample-env\Scripts\activate
        python -m pip install opencv-python
        $pakname = Get-ChildItem -Name ..\sdk\python\id3devices-*-cp311-cp311-win_amd64.whl
        python -m pip install ..\sdk\python\$pakname
        python sample_capture.py
        deactivate
    }
    catch
    {
        Write-Output "Error in python"
        throw
    }
    finally
    {
        Pop-Location
    }
}

try
{
    switch  ($language)
    {
        "all" {
            # cpp
            BuildCppAll
            BuildCppMFC

            # dotnet
            BuildDotnet

            # python
            BuildPython
        }
        "dotnet" {
            BuildDotnet
        }
        "python" {
            BuildPython
        }
        "cpp" {
            BuildCppAll
        }
        "cpp-cv" {
            BuildCppOpenCv
        }
        "cpp-mfc" {
            BuildCppMFC
        }
        default { Write-Output "Unknown language $($language)" }
    }
    Pause
}
catch
{
}
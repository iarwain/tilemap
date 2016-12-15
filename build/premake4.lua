-- This premake script should be used with the orx-customized version of premake4.
-- Its Mercurial repository can be found at https://bitbucket.org/orx/premake-stable.
-- A copy, including binaries, can also be found in the extern/premake folder of any orx distributions.

--
-- Globals
--

function initconfigurations ()
    return
    {
        "Debug",
        "Release"
    }
end

function initplatforms ()
    if os.is ("windows") then
        if string.lower(_ACTION) == "vs2013"
        or string.lower(_ACTION) == "vs2015" then
            return
            {
                "x64",
                "x32"
            }
        else
            return
            {
                "Native"
            }
        end
    elseif os.is64bit () then
        return
        {
            "x64",
            "x32"
        }
    else
        return
        {
            "x32",
            "x64"
        }
    end
end

function defaultaction (name, action)
   if os.is (name) then
      _ACTION = _ACTION or action
   end
end

defaultaction ("windows", "vs2015")
defaultaction ("linux", "gmake")
defaultaction ("macosx", "gmake")

if os.is ("macosx") then
    osname = "mac"
else
    osname = os.get()
end

destination = "./" .. osname .. "/" .. _ACTION


--
-- Solution: tilemap
--

solution "Tilemap"

    language ("C++")

    location (destination)

    kind ("ConsoleApp")

    configurations
    {
        initconfigurations ()
    }

    platforms
    {
        initplatforms ()
    }

    includedirs
    {
        "../include",
        "../include/orx",
        "$(ORX)/include"
    }

    flags
    {
        "NoPCH",
        "NoManifest",
        "EnableSSE2",
        "FloatFast",
        "NoNativeWChar",
        "NoExceptions",
        "Symbols",
        "StaticRuntime"
    }

    configuration {"not windows"}
        flags {"Unicode"}

    configuration {"*Debug*"}
        targetsuffix ("d")
        defines {"__orxDEBUG__"}
        links {"orxd"}

    configuration {"*Release*"}
        flags {"Optimize", "NoRTTI"}
        links {"orx"}

    configuration {"not macosx", "*Release*"}
        kind ("WindowedApp")


-- Linux

    -- This prevents an optimization bug from happening with some versions of gcc on linux
    configuration {"linux", "not *Debug*"}
        buildoptions {"-fschedule-insns"}


-- Mac OS X

    configuration {"macosx"}
        buildoptions
        {
            "-mmacosx-version-min=10.6",
            "-gdwarf-2",
            "-Wno-write-strings"
        }
        linkoptions
        {
            "-mmacosx-version-min=10.6",
            "-dead_strip"
        }

    configuration {"macosx", "x32"}
        buildoptions
        {
            "-mfix-and-continue"
        }


-- Windows


--
-- Project: tilemap
--

project "Tilemap"

    files
    {
        "../src/**.c",
        "../include/**.h",
        "../data/*.ini"
    }
    targetname ("tilemap")


-- Linux

    configuration {"linux"}
        linkoptions {"-Wl,-rpath ./", "-Wl,--export-dynamic"}
        links
        {
            "dl",
            "m",
            "rt"
        }

    configuration {"linux", "x32"}
        libdirs {"../lib/linux/x32"}
        targetdir ("../bin/linux/x32")

    configuration {"linux", "x64"}
        libdirs {"../lib/linux/x64"}
        targetdir ("../bin/linux/x64")


-- Mac OS X

    configuration {"macosx"}
        links
        {
            "Foundation.framework",
            "AppKit.framework"
        }
        libdirs {"../lib/mac"}
        targetdir ("../bin/mac")


-- Windows

    configuration {"windows"}
        links
        {
            "winmm"
        }

    configuration {"windows", "x64"}
        libdirs {"../lib/windows/x64"}
        targetdir ("../bin/windows/x64")

    configuration {"windows", "not x64"}
        libdirs {"../lib/windows/x32"}
        targetdir ("../bin/windows/x32")


-- Common

    configuration {}
        libdirs {"$(ORX)/lib/dynamic"}

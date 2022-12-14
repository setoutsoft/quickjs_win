
(function()
  -- generate "quickjs-version.h" using VERSION file
  local file = io.open("VERSION", "r")
  local vers = file:read()
  file:close()
  vars = vers:gsub("%s+", "")
  file = io.open("quickjs-version.h", "w+")
  file:write("#define QUICKJS_VERSION \"" .. vers .. "\"")
  file:close()
end)()  


newoption {
   trigger     = "jsx",
   description = "Will add JSX support"
}

workspace "quickjs"
	-- Premake output folder
	location(path.join(".build", _ACTION))

  defines {
  	  "JS_STRICT_NAN_BOXING", -- this option enables x64 build on Windows/MSVC
               "CONFIG_BIGNUM",
	  "_CRT_NONSTDC_NO_WARNINGS"
    } 

  if _OPTIONS["jsx"] then 
    defines { "CONFIG_JSX" } -- native JSX support - enables JSX literals
  end


	platforms { "x86", "x64", "arm32", "arm64"  } 

	-- Configuration settings
	configurations { "Debug", "Release" }

	filter "platforms:x86"
  	architecture "x86"
	filter "platforms:x64"
  	architecture "x86_64"  
	filter "platforms:arm32"
  	architecture "ARM"  
	filter "platforms:arm64"
  	architecture "ARM64"  

	filter "system:windows"
  	removeplatforms { "arm32" }  

	-- Debug configuration
	filter { "configurations:Debug" }
		defines { "DEBUG" }
		symbols "On"
		optimize "Off"
                          staticruntime "on"

	-- Release configuration
	filter { "configurations:Release" }
		defines { "NDEBUG" }
		optimize "Speed"
		inlining "Auto"
                           staticruntime "on"

	filter { "language:not C#" }
		defines { "_CRT_SECURE_NO_WARNINGS" }
		buildoptions { "/std:c++latest" }
		systemversion "latest"

	filter { }
		targetdir ".bin/%{cfg.buildcfg}/%{cfg.platform }"
		exceptionhandling "Off"
		rtti "Off"
		--vectorextensions "AVX2"

-----------------------------------------------------------------------------------------------------------------------

project "quickjs"
	language "C"
	kind "SharedLib"
	defines{"QJS_BUILD"}
	files {
                           "cutils.h",
		"cutils.c",
		"libregexp.c",
		"libunicode.c",
		"quickjs.c",
		"quickjs-libc.c",
		"libbf.c",
		"libregexp.h",
		"libregexp-opcode.h",
		"libunicode.h",
		"libunicode-table.h",
		"list.h",
		"quickjs.h",
		"quickjs-api.h",
		"quickjs-atom.h",
		"quickjs-libc.h",
		"quickjs-opcode.h",
		"quickjs-jsx.h",
	}

-----------------------------------------------------------------------------------------------------------------------

project "qjsc"
	language "C"
	kind "ConsoleApp"
	links { "quickjs" }
	files {
		"qjsc.c"
	}

-----------------------------------------------------------------------------------------------------------------------

project "qjs"
	language "C"
	kind "ConsoleApp"
	links { "quickjs" }
	dependson { "qjsc" }
	files {
		"qjs.c",
		"repl.js",
		"repl.c",
		"qjscalc.js",
		"qjscalc.c"
	}

-- Compile repl.js and save bytecode into repl.c
prebuildcommands { "\"%{cfg.buildtarget.directory}/qjsc.exe\" -c -o \"../../repl.c\" -m \"../../repl.js\"" }
prebuildcommands { "\"%{cfg.buildtarget.directory}/qjsc.exe\" -c -o \"../../qjscalc.c\" -m \"../../qjscalc.js\"" }

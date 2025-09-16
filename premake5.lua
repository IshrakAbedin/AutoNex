-- premake5.lua
workspace "AutoNex"
   configurations { "Debug", "Release" }
   
   project "AutoNex"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   architecture "x64"
   targetdir "bin/%{cfg.buildcfg}"

   includedirs {
         "./test",
	      "./include/autonex"
    }

   files { "**.h", "**.hpp", "**.cpp", "**.cc", "**.cx", "**.c" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
workspace "tsra"
   configurations { "debug", "release" }
   language "C"
   files { "scheme.c", "scheme.h", "opdefines.h", "common.h", "ts_std.h" }

   filter "configurations:Debug"
      symbols "On"

   filter "configurations:Release"
      optimize "On"

   newoption {
       trigger = "use-math",
       description = "Include math routines"
   }

   newoption {
       trigger = "use-char-classifiers",
       description = "Include character classifier procedures"
   }

   newoption {
       trigger = "use-ascii-names",
       description = "Enable extended character notation based on ASCII names"
   }

   newoption {
       trigger = "use-string-ports",
       description = "Enable string ports"
   }

   newoption {
       trigger = "use-error-hook",
       description = "Force system errors through user-defined error handling"
   }

   newoption {
       trigger = "use-tracing",
       description = "Enable use of TRACING"
   }

   newoption {
       trigger = "use-colon-hook",
       description = "Enable use of qualified identifiers"
   }

   newoption {
       trigger = "use-strcasecmp",
       description = "Define stricmp as strcasecmp"
   }

   newoption {
       trigger = "stdio-adds-cr",
       description = "Inform tsra that stdio translates \"\\n\" to \"\\r\\n\""
   }

   newoption {
       trigger = "use-dl",
       description = "Enable dynamically loaded routines"
   }

   newoption {
       trigger = "use-plist",
       description = "Enable property lists"
   }

   newoption {
       trigger = "use-stack",
       description = "Enable 'cons' stack"
   }
   
   newoption {
       trigger = "show-error-line",
       description = "Show line number when an error occurred"
   }
   
   newoption {
       trigger = "use-interface",
       description = "Use dynamic interface"
   }

   filter "options:use-math"
      defines { "USE_MATH=1" }

   filter "options:use-char-classifiers"
      defines { "USE_CHAR_CLASSIFIERS=1" }

   filter "options:use-ascii-names"
      defines { "USE_ASCII_NAMES=1" }

   filter "options:use-string-ports"
      defines { "USE_STRING_PORTS=1" }

   filter "options:use-error-hook"
      defines { "USE_ERROR_HOOK=1" }

   filter "options:use-tracing"
      defines { "USE_TRACING=1" }

   filter "options:use-colon-hook"
      defines { "USE_COLON_HOOK=1" }

   filter "options:use-strcasecmp"
      defines { "USE_STRCASECMP=1" }

   filter "options:stdio-adds-cr"
      defines { "STDIO_ADDS_CR=1" }

   filter "options:use-plist"
      defines { "USE_PLIST=1" }

   filter "options:use-stack"
      defines { "USE_STACK=1" }

   filter "options:show-error-line"
      defines { "SHOW_ERROR_LINE=1" }

   filter "options:use-interface"
      defines { "USE_INTERFACE=1" }
   
   filter "system:linux or macosx"
      files { "ts_std_unix.c", "ts_std_unix.h" }

   filter "system:windows"
      files { "ts_std_win.h" }
   
   filter "options:use-dl"
      files { "ts_dl.c", "ts_dl.h" }

      filter "system:linux or macosx"
         files { "ts_dl_unix.c", "ts_dl_unix.c" }
         links { "dl" }

      filter "system:windows"
         files { "ts_dl_win.c", "ts_dl_win.h" }

   filter "options:use-math"
      filter "system:linux or macosx"
         links { "m" }

project "tsra"
   kind "ConsoleApp"
   defines { "STANDALONE=1" }
   files { "main.c" }

project "libtsra"
   kind "StaticLib"
   files { "library.c" }
   targetname "tsra"
   pic "On"

project "tsradll"
   kind "SharedLib"
   files { "library.c" }
   targetname "tsra"

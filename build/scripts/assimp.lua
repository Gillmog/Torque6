function assimp()
    project "assimp"
        location (LIB_PROJECT_DIR)
        targetdir (LIB_BUILD_DIR)

        targetname "assimp"
        language "C++"
        kind "StaticLib"

        defines {
            "ASSIMP_BUILD_NO_C4D_IMPORTER",
            "ASSIMP_BUILD_NO_OPENGEX_IMPORTER"
        }

        includedirs {
            SRC_DIR,
            path.join(LIB_DIR, "assimp/code/BoostWorkaround"),
            path.join(LIB_DIR, "assimp/contrib/openddlparser/include"),
            path.join(LIB_DIR, "assimp/contrib/rapidjson/include"),
            path.join(LIB_DIR, "assimp/include"),
        }

        files {
            path.join(LIB_DIR, "assimp/code/**.h"),
            path.join(LIB_DIR, "assimp/code/**.cpp"),
            path.join(LIB_DIR, "assimp/contrib/clipper/clipper.hpp"),
            path.join(LIB_DIR, "assimp/contrib/clipper/clipper.cpp"),
            path.join(LIB_DIR, "assimp/contrib/ConvertUTF/ConvertUTF.c"),
            path.join(LIB_DIR, "assimp/contrib/ConvertUTF/ConvertUTF.h"),
            path.join(LIB_DIR, "assimp/contrib/irrXML/**.h"),
            path.join(LIB_DIR, "assimp/contrib/irrXML/**.cpp"),
            path.join(LIB_DIR, "assimp/contrib/poly2tri/poly2tri/common/**.h"),
            path.join(LIB_DIR, "assimp/contrib/poly2tri/poly2tri/common/**.cc"),
            path.join(LIB_DIR, "assimp/contrib/poly2tri/poly2tri/sweep/**.h"),
            path.join(LIB_DIR, "assimp/contrib/poly2tri/poly2tri/sweep/**.cc"),
            path.join(LIB_DIR, "assimp/contrib/poly2tri/poly2tri/poly2tri.h"),
            path.join(LIB_DIR, "assimp/contrib/unzip/**.h"),
            path.join(LIB_DIR, "assimp/contrib/unzip/**.c"),
            path.join(LIB_DIR, "assimp/contrib/zlib/**.h"),
            path.join(LIB_DIR, "assimp/contrib/zlib/**.c"),
            path.join(LIB_DIR, "assimp/include/assimp/**.hpp"),
            path.join(LIB_DIR, "assimp/include/assimp/**.h"),
        }

        configuration { "windows", "x32", "Release" }
            targetdir (LIB_BUILD_DIR .. "/windows.x32.release")

        configuration { "windows", "x32", "Debug" }
            targetdir (LIB_BUILD_DIR .. "/windows.x32.debug")

        configuration { "windows", "x64", "Release" }
            targetdir (LIB_BUILD_DIR .. "/windows.x64.release")

        configuration { "windows", "x64", "Debug" }
            targetdir (LIB_BUILD_DIR .. "/windows.x64.debug")

        configuration "Debug"
            defines { "TORQUE_DEBUG" }
            flags   { "Symbols" }

        configuration "vs*"
            defines { "_CRT_SECURE_NO_WARNINGS" }

        configuration "vs2015"
            windowstargetplatformversion "10.0.10240.0"

        configuration "windows"
            links { "ole32" }

        configuration "linux"
            links { "dl" }

        configuration "linux or bsd"
            links        { "m" }
            linkoptions  { "-rdynamic " }
            buildoptions { "-fPIC" }

        configuration "macosx"

        configuration { "macosx", "gmake" }
            buildoptions { "-mmacosx-version-min=10.4" }
            linkoptions  { "-mmacosx-version-min=10.4" }
end

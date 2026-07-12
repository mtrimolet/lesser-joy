package("simpleble", function()
    set_kind("library")
    set_homepage("https://github.com/simpleble/simpleble")
    set_description("The all-in-one Bluetooth library for MacOS, iOS, Windows, Linux and Android.")
    set_license("BUSL-1.1")

    add_urls("https://github.com/simpleble/simpleble/archive/refs/tags/$(version).tar.gz",
             "https://github.com/simpleble/simpleble.git")

    add_versions("v0.14.0", "59db8ff215c917669e2678a6353319eae32ffe35c85b1e9ca7912bdee1d6167b")

    -- if is_plat("windows", "mingw") then
    --     add_syslinks("dbghelp")
    -- elseif is_plat("linux", "cross") then
    --     add_syslinks("dl")
    -- end

    -- add_deps("cmake")
    -- if not is_plat("windows") then
    --     add_deps("libdwarf")
    -- end

    on_install("linux", "macosx", "windows", "mingw", "cross", function (package)
        local configs = {
            kind = package:config("shared") and "shared" or "static",
            -- moduleonly = package:config("moduleonly"),
        }
        io.writefile(
            "simpleble/include/simpleble/export.h",
            [[
                #pragma once

                #define SIMPLEBLE_EXPORT
            ]])
        io.writefile(
            "xmake.lua",
            [[
                add_rules("mode.release", "mode.debug")
                
                target("simpleble")
                    set_languages("c++23")
                    set_kind("static")

                    add_defines("FMT_HEADER_ONLY")
                    add_defines("_CRT_STDIO_ISO_WIDE_SPECIFIERS=1")

                    add_defines("SIMPLEBLE_VERSION=\"0.14.0\"")

                    add_defines("SIMPLEBLE_BACKEND_PLAIN=0")
                    add_defines("SIMPLEBLE_BACKEND_LINUX=0")
                    add_defines("SIMPLEBLE_BACKEND_WINDOWS=1")
                    add_defines("SIMPLEBLE_BACKEND_ANDROID=0")
                    add_defines("SIMPLEBLE_BACKEND_MACOS=0")
                    add_defines("SIMPLEBLE_BACKEND_IOS=0")
                    
                    add_headerfiles("simpleble/include/(**.h)", "dependencies/external/(**.h)")
                    add_includedirs(
                        "simpleble/src",
                        "simpleble/include",
                        "simpleble/src/builders",
                        "simpleble/src/external",
                        "simpleble/src/backends/common",
                        "simpleble/src/backends/dongl",
                        "simpleble/src/frontends/safe",
                        "dependencies/external",
                        "dependencies/internal/include"
                    )
                    add_files(
                        "simpleble/src/*.cpp",
                        "simpleble/src/builders/**.cpp",
                        "simpleble/src/external/**.cpp",
                        "simpleble/src/frontends/**.cpp",
                        "simpleble/src/backends/common/**.cpp",
                        "simpleble/src/backends/dongl/protocol/**.c",
                        "simpleble/src/backends/dongl/*.cpp",
                        "simpleble/src/backends/dongl/serial/**.cpp",
                        "simpleble/src/backends/dongl/usb/UsbHelper.cpp",
                        "simpleble/src/backends/dongl/usb/UsbHelperWindows.cpp",
                        "simpleble/src/backends/windows/**.cpp",
                        "dependencies/internal/src/**.c"
                        -- "dependencies/internal/src/**.cpp"
                    )
            ]]
        )
        import("package.tools.xmake").install(package, configs)
        -- if not package:config("shared") then
        --     package:add("defines", "SIMPLEBLE_STATIC_DEFINE")
        -- end

        -- os.cp("simpleble/CMakeLists.txt", "CMakeLists.txt")
        -- io.replace("CMakeLists.txt", "${CMAKE_CURRENT_SOURCE_DIR}/../", "", {plain = true})
        -- io.replace("CMakeLists.txt", "${CMAKE_CURRENT_SOURCE_DIR}/", "simpleble/", {plain = true})
        -- io.replace("CMakeLists.txt", "${CMAKE_CURRENT_LIST_DIR}/../", "", {plain = true})
        -- io.replace("CMakeLists.txt", "${CMAKE_CURRENT_LIST_DIR}/", "simpleble/", {plain = true})
        -- io.replace("CMakeLists.txt", "CXX_STANDARD 17", "CXX_STANDARD 26", {plain = true})

        -- local configs = {
        --     "-DSIMPLEBLE_TEST=OFF",
        -- }
        -- table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:is_debug() and "Debug" or "Release"))
        -- table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        -- import("package.tools.cmake").install(package, configs)
    end)
end)

add_requires("simpleble")

target("lesserjoy-bt", function()
    set_languages("c++26")
    add_files("src/*.cpp")
    set_policy("build.c++.modules", true)
    add_cxflags("-fcolor-diagnostics", "-fansi-escape-codes")

    set_runtimes("c++_static")

    add_packages("simpleble", "frozen")

    add_rules(stormkit_rule_prefix .. "stormkit::application")
    set_values("stormkit.components", { "stormkit", "log" })

    -- on_run(function(target)
    --     import("privilege.sudo")
    --     sudo.exec(target:targetfile())
    -- end)
end)

rule("install-wdk", function()
    add_imports("core.project.config")

    on_load(function(target)
        import("lib.detect.find_tool")

        if config.get("wdk") then
          return
        end

        local nuget_packages_dir = path.join(os.projectdir(), "packages")

        function find_dir(pattern)
            for _, dir in ipairs(os.dirs(pattern)) do
                return dir
            end
            return nil
        end

        local wdk_package_id = "Microsoft.Windows.WDK." .. (target:is_arch("arm64") and "arm64" or "x64")
        local wdk_package_dir = find_dir(path.join(nuget_packages_dir, wdk_package_id) .. ".*")

        if not wdk_package_dir then
            local nuget = find_tool("nuget", { check = function(tool) os.run("%s help", tool) end })
            assert(nuget, "nuget not found!")

            print("installing wdk")
            os.execv(nuget.program, {
                "restore",
                path.join(os.projectdir(), "packages.config"),
                "-PackagesDirectory",
                nuget_packages_dir,
                "-Verbosity",
                "quiet"
            }, {})

            wdk_package_dir = find_dir(path.join(nuget_packages_dir, wdk_package_id) .. ".*")
            assert(wdk_package_dir, "WDK package not found!")
        end

        target:add("values", "wdk", wdk_package_dir)
    end)
end)

add_rules("mode.debug", "mode.release")

add_rules("install-wdk")

includes("stormkit-driver/xmake.lua")

includes("driver/xmake.lua")
includes("bluetooth-driver/xmake.lua")

includes("qt-frontend/xmake.lua")
includes("cli-frontend/xmake.lua")

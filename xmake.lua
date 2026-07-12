rule("install-wdk", function()
    on_load(function()
        import("lib.detect.find_tool")

        local nuget = find_tool("nuget", { check = function(tool) os.run("%s help", tool) end })
        assert(nuget, "nuget not found!")

        if not os.isdir("packages") then
            print("pulling wdk")
            os.execv(nuget.program, {
                "restore",
                path.join(os.projectdir(), "packages.config"),
                "-PackagesDirectory",
                path.join(os.projectdir(), "packages"),
            }, {})
        end
    end)
end)

add_rules("mode.debug", "mode.release", "mode.releasedbg")

add_repositories("tapzcrew-repo https://github.com/tapzcrew/xmake-repo main")

option("sanitizers", { default = false, category = "root menu/build" })
option("mold", { default = false, category = "root menu/build" })
option("lto", { default = true, category = "root menu/build" })

option("bt", { description = "build bt service", default = true, category = "root menu/services" })
option("cli", { description = "build cli frontend", default = true, category = "root menu/frontends" })
option("gui", { description = "build gui frontend", default = true, category = "root menu/frontends" })

option("stormkit", { description = "local stormkit folder", type = "string", category = "root menu/support" })
option("on_ci", { default = false, category = "root menu/support" })
option("compile_commands", { default = false, category = "root menu/support" })
option("vsxmake", { default = false, category = "root menu/support" })
option("devmode", {
    category = "root menu/support",
    deps = { "lto", "sanitizers" },
    defines = { "INCREMENTAL_BUILD" },
    after_check = function(option)
        if option:enabled() then
            for _, name in ipairs({ "sanitizers" }) do
                option:dep(name):enable(true)
            end
            option:dep("lto"):enable(false)
        end
    end,
})

if get_config("devmode") then set_policy("build.c++.modules.hide_dependencies", true) end
if get_config("vsxmake") then add_rules("plugin.vsxmake.autoupdate") end
if get_config("compile_commands") then
    add_rules("plugin.compile_commands.autoupdate", { outputdir = "build", lsp = "clangd" })
end

set_policy("build.c++.modules.two_phases", false)

local stormkit_dep_name = "stormkit"
stormkit_rule_prefix = "@stormkit/"
if has_config("stormkit") then
    includes("xmake/StormKit.xmake.lua")
    stormkit_dep_name = "dev_stormkit"
end

local windows_runtime = "MT"
-- if is_mode("debug") then windows_runtime = windows_runtime .. "d" end

set_runtimes({ windows_runtime, get_config("runtimes") })

add_ldflags("-fuse-ld=lld-link", { force = true })
add_shflags("-fuse-ld=lld-link", { force = true })

set_policy("check.auto_ignore_flags", true)

add_requires("frozen", { system = false, configs = { modules = true, std_import = true, cpp = "latest" } })
add_requires("unordered_dense", { system = false, configs = { modules = true, std_import = true } })
add_requires("nontype_functional main")

add_requires(stormkit_dep_name, {
    configs = {
        log = true,
        wsi = false,
        math = false,
        entities = false,
        image = false,
        gpu = false,
        lua = false,

        examples = false,
        tests = false,
        tools = false,

        shared = false,
        debug = is_mode("debug"),
        lto = get_config("lto"),

        defines = { "_CRT_STDIO_ISO_WIDE_SPECIFIERS=1" },
    },
    version = "dev",
    alias = "stormkit",
})

-- includes("stormkit-driver/xmake.lua")
includes("driver/xmake.lua")
if get_config("bt") then includes("bt-svc/xmake.lua") end
if get_config("gui") then includes("qt-frontend/xmake.lua") end
if get_config("cli") then includes("cli-frontend/xmake.lua") end

target("lesser-joy-bluetooth-driver", function()
    set_languages("c++26")

    add_deps("stormkit-driver")

    add_files("*.rc")
    add_files("*.inf")
    add_rules("wdk.driver", "wdk.env.kmdf")
    add_values("wdk.kmdf.sdkver", "1.23")

    add_files("src/*.cpp", "src/*.cppm")
    set_policy("build.c++.modules", true)
    set_policy("build.c++.modules.std", false)
    set_exceptions("no-cxx")
end)

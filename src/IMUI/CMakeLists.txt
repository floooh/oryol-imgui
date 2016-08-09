#-------------------------------------------------------------------------------
#   oryol ImGui integration module
#-------------------------------------------------------------------------------
fips_begin_module(IMUI)
    fips_vs_warning_level(3)
    fips_files(
        IMUI.h IMUI.cc
        IMUISetup.h
        imguiWrapper.h imguiWrapper.cc
    )
    oryol_shader(IMUIShaders.shd)
    fips_deps(Gfx Input imgui)
fips_end_module()

project('protoflecs', 'cpp')
cpp = meson.get_compiler('cpp')

flecs_lib = subproject('flecs')
flecs_dep = flecs_lib.get_variable('flecs_dep')

# (Meson requires an absolute path for find_library().)
libdir = meson.current_source_dir() + './subprojects/rlImGui/bin/Release/'

precompiled_rlImGui_dep = cpp.find_library('rlImGui', dirs : '/home/evergold/Documents/protoflecs/subprojects/rlImGui/bin/Release/')

deps = [flecs_dep, precompiled_rlImGui_dep, dependency('raylib', version : '>=5.0')]
executable('main', 'src/main.cpp', dependencies : deps)
executable('editor', 'src/editor.cpp', dependencies : deps)
executable('vamp', 'src/vamp/vamp.cpp', dependencies : deps)

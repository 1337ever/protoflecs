project('protoflecs', 'cpp')

flecs_lib = subproject('flecs')
flecs_dep = flecs_lib.get_variable('flecs_dep')

deps = [flecs_dep, dependency('raylib', version : '>=5.0')]
executable('main', 'src/main.cpp', dependencies : deps)
executable('editor', 'src/editor.cpp', dependencies : deps)
executable('vamp', 'src/vamp/vamp.cpp', dependencies : deps)
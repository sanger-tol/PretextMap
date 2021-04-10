project('SamHaplotag', ['c', 'cpp'],
    license: 'MIT',
    meson_version: '>=0.57.1',
    version: '0.1.5'
)

thread_dep = dependency('threads')

base_flags = ['-Ofast']
extensions = {'.avx2': ['-mavx2'], '.avx': ['-mavx'], '.sse42': ['-msse4.2'], '.sse41': ['-msse4.1'], '.noext': []}

foreach ext, flag : extensions
    flags = base_flags + flag
    executable('PretextMap' + ext, 'PretextMap.cpp', dependencies : thread_dep, install : true, cpp_args : flags, link_with : 
        [   static_library('deflate' + ext, 'libdeflate/lib/deflate_compress.c', 'libdeflate/lib/deflate_decompress.c', 'libdeflate/lib/utils.c', c_args : flags, link_with : 
            [   static_library('deflate_x86' + ext, 'libdeflate/lib/x86/cpu_features.c', c_args : flags),
                static_library('deflate_arm' + ext, 'libdeflate/lib/arm/cpu_features.c', c_args : flags)]),
            static_library('mpc' + ext, 'mpc/mpc.c', c_args : flags)])
endforeach

e = executable('PretextMap', 'wrapper/SIMDExtensionWrapper.cpp', install : true, cpp_args : base_flags + ['-DProgramName=PretextMap'])

test('Test', e)

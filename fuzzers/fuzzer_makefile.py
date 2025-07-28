import powermake
import powermake.compilers
import powermake.linkers


def on_build(config: powermake.Config):
    files = powermake.filter_files(powermake.get_files("../requests/**/*.c", "*.c"), "**/easy_tcp_tls.c")

    config.c_compiler = powermake.compilers.CompilerClang()
    config.linker = powermake.linkers.LinkerClang()

    if not config.debug:
        config.add_flags("-flto")

    config.add_flags("-ffuzzer", "-fsecurity")
    config.add_includedirs("../requests")
    config.set_optimization("-O0")

    objects = powermake.compile_files(config, files)

    powermake.link_files(config, objects)


powermake.run("fuzzer", build_callback=on_build)

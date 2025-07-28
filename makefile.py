import powermake
import typing as T


def on_build(config: powermake.Config):
    config.add_includedirs("./requests")
    config.add_flags("-Wsecurity")

    if config.target_is_mingw():
        config.remove_flags("-fanalyzer")  # for some reason, -fanalyzer under MinGW is full of false positive.

    if config.target_is_windows():
        config.add_shared_libs("ssl", "crypto", "crypt32", "ws2_32")
        config.add_ld_flags("-static")
    else:
        config.add_shared_libs("ssl", "crypto")

    files = powermake.get_files("requests/**/*.c", "test.c")

    objects = powermake.compile_files(config, files)

    powermake.archive_files(config, powermake.filter_files(objects, "**/test.*"))

    powermake.link_shared_lib(config, powermake.filter_files(objects, "**/test.*"))

    powermake.link_files(config, objects, executable_name="test_requests")


def on_install(config: powermake.Config, location: T.Union[str, None]):
    config.add_exported_headers("requests/requests.h", subfolder="requests")

    powermake.default_on_install(config, location)


powermake.run("requests", build_callback=on_build)
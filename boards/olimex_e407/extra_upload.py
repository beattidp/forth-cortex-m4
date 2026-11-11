import os
from os.path import isdir, join
from os import makedirs

Import("env")

def _jlink_custom_cmd_script(env, source):
    build_dir = env.subst("$BUILD_DIR")
    if not isdir(build_dir):
        makedirs(build_dir)
    script_path = join(build_dir, "upload.jlink")
    commands = [
        "connect",
        "h",
        "loadbin %s, 0x08000000" % source,
        "r",
        "g",
        "q"
    ]
    with open(script_path, "w") as fp:
        fp.write("\n".join(commands) + "\n")
    return script_path

env.Replace(
    __jlink_cmd_script=_jlink_custom_cmd_script,
    UPLOADERFLAGS=[
        "-device", "STM32F407ZG",
        "-speed", env.GetProjectOption("debug_speed", "4000"),
        "-if", "jtag",
        "-jtagconf", "-1,-1",
        "-autoconnect", "1",
        "-NoGui", "1"
    ],
    UPLOADCMD='$UPLOADER $UPLOADERFLAGS -CommanderScript "${__jlink_cmd_script(__env__, SOURCE)}"'
)

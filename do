#!/usr/bin/env python3

# https://www.silabs.com/documents/public/software/CP210x_Universal_Windows_Driver.zip

import os
import sys
import subprocess
import re
from datetime import datetime
from os.path import exists
import argparse
import json
from time import time
from serial.tools import list_ports, miniterm

settings = dict()
has_jlink = False

def error_exit(mess):
    print(f"*** Error. {mess}")
    sys.exit(1)

#--------------------------------------------------------------------


def exec_command(com):
    sub_proc = subprocess.run(com, universal_newlines=True, shell=True)
    return sub_proc.returncode

#--------------------------------------------------------------------

def check_jlink():
    global has_jlink
    sub_proc = subprocess.run("nrfjprog --ids", universal_newlines=True, shell=True,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    has_jlink = sub_proc.returncode == 0 and len(sub_proc.stdout) > 0

#--------------------------------------------------------------------

def find_uart0():
    for port in list_ports.comports():
        if re.search("CP210.+Interface 0", str(port)):
            return port.device
    error_exit("Failed to detect the serial port.\nIs the unit connected?")

#--------------------------------------------------------------------

def get_exe_file(args, signed):
    file_ext = ".hex" if has_jlink else ".bin"
    file_name = f"{args.build_dir}/zephyr/zephyr"
    file_type = "_signed" if signed else ""
    return file_name + file_type + file_ext

#--------------------------------------------------------------------

def build(args):
    print(f"=== {args.example} ===")
    os.chdir(args.example)
    com = f"west build --board nrf5340dk_nrf5340_cpuapp --build-dir {args.build_dir}"
    if args.pristine:
        com += " --pristine"
    start = time()
    if exec_command(com) == 0:
        exe_file = get_exe_file(args, False)
        if not args.no_bootloader and os.path.getmtime(exe_file) > start:
            mcuboot_dir = settings['ncs_dir'] + "/bootloader/mcuboot"
            sign_com = (f"{sys.executable} {mcuboot_dir}/scripts/imgtool.py sign"
                        f" --key {mcuboot_dir}/root-rsa-2048.pem"
                        " --header-size 0x200 --align 4 --version 0.0.0+0"
                        " --pad-header --slot-size 0xe0000"
                        f" {exe_file} {get_exe_file(args, True)}")
            exec_command(sign_com)
            return 1
    else:
        error_exit("Build failed")
    return 0

#--------------------------------------------------------------------

def flash(args):
    if build(args) == 0 and args.when_changed:
        return
    flash_file = get_exe_file(args, not args.no_bootloader)
    if has_jlink:
        print("Flashing using jlink")
        com = f"nrfjprog -f nrf53 --program {flash_file} --sectorerase --verify --reset"
    elif not args.no_bootloader:
        print("Flashing using serial port")
        uart0 = find_uart0()
        print("Restart the XPLR-IOT-1 simultaneously pressing button 1")
        input("Press return when ready: ")
        serial_flash_com = f"newtmgr --conntype serial --connstring \"{uart0},baud=115200\""
        com = f"{serial_flash_com} image upload {flash_file}"
    else:
        error_exit("Can not do serial flash without bootloader")
    exec_command(com)
    if not has_jlink:
        print("Restarting")
        exec_command(f"{serial_flash_com} reset")

#--------------------------------------------------------------------

def monitor(args):
    sys.argv = [sys.argv[0]]
    sys.argv.append(find_uart0())
    sys.argv.append("115200")
    sys.argv.append("--raw")
    miniterm.main()

#--------------------------------------------------------------------

def reset(args):
    if has_jlink:
        exec_command("nrfjprog -f nrf53 --reset")
    else:
        exec_command(f"newtmgr --conntype serial --connstring \"{uart0},baud=115200\"")

#--------------------------------------------------------------------

def run(args):
    flash(args)
    monitor(args)

#--------------------------------------------------------------------

def flash_bootloader(args):
    print("Flashing serial mcuboot bootloader...")
    exec_command(
        "nrfjprog -f nrf53 --program xplriot1/mcuboot_serial.hex --sectorerase --reset --verify")

#--------------------------------------------------------------------

def debug(args):
    exec_command(f"west debug --build-dir {args.build_dir}")

#--------------------------------------------------------------------

settings_file_name = ".settings"

def read_settings():
    global settings
    if exists(settings_file_name):
        settings = json.loads(open(settings_file_name).read())

#--------------------------------------------------------------------

def save(args):
    try:
        open(settings_file_name, 'w').write(json.dumps(settings))
    except Exception as e:
        error_exit(f"Failed to save settings file {settings_file_name}\n{e}")

#--------------------------------------------------------------------

def show_option(args):
    if args.operation[1] == "build_dir":
        res = settings['build_dir']
    elif args.operation[1] == "gcc_dir":
        res = os.path.expandvars(settings['gcc_dir'])
    elif args.operation[1] == "exe_file":
        res = os.path.basename(get_exe_file(args, not args.no_bootloader))
    elif args.operation[1] == "list_ex":
        res = ""
        for ex in examples:
            res += ex + "\n"
    else:
        error_exit(f"No such option: \"args.operation[1]\"")
    print(res)

#--------------------------------------------------------------------

def vscode(args):
    build(args)
    os.chdir("../")
    vscode_dir = ".vscode"
    os.makedirs(vscode_dir, exist_ok=True)
    comp_path = re.sub(r"\$HOME", "${env:HOME}", settings['gcc_dir'])
    properties = '{\n"version": 4,\n"configurations": ['
    for ex in examples:
        properties += (
        '    {\n'
        f'      "name": "{ex}",\n'
        f"      \"compilerPath\": \"{comp_path}/bin/arm-none-eabi-gcc\",\n"
        '      "cStandard": "c11",\n'
        '      "cppStandard": "c++14",\n'
        f"      \"includePath\": [\n"
        '         "${workspaceFolder}/**",\n'
        f"         \"{os.path.expandvars(settings['ubxlib_dir'])}/**\"\n"
        "       ],\n"
        f"      \"compileCommands\": \"{settings['build_dir']}/{ex}/compile_commands.json\"\n"
        '    },\n'
        )
    properties = re.sub(r",\n$", "\n", properties)
    properties += "  ]\n}\n"
    with open(vscode_dir + "/c_cpp_properties.json", "w") as f:
        f.write(properties)

    exec_command(f"code examples.code-workspace -g {args.example}/src/main.c")

#--------------------------------------------------------------------

def set_env():
    os.environ['ZEPHYR_BASE'] = os.path.expandvars(settings['ncs_dir']) + "/zephyr"
    if 'gcc_dir' in settings:
        os.environ['GNUARMEMB_TOOLCHAIN_PATH'] = os.path.expandvars(
            settings['gcc_dir'])
    os.environ['ZEPHYR_TOOLCHAIN_VARIANT'] = "gnuarmemb"
    os.environ['UBXLIB_DIR'] = os.path.expandvars(settings['ubxlib_dir'])
    if not settings['no_bootloader']:
        os.environ['USE_BOOTLOADER'] = "1"

#--------------------------------------------------------------------

def check_directories(args):
    global settings
    # nRFConnect directory
    if args.ncs_dir != None:
        settings['ncs_dir'] = args.ncs_dir
    elif not 'ncs_dir' in settings:
        ncs_dir = os.environ['HOME'] if 'linux' in sys.platform else os.environ['HOMEDRIVE']
        ncs_dir += "/ncs"
        version = ""
        if exists(ncs_dir):
            # Find latest installed version
            for entry in os.scandir(ncs_dir):
                if entry.is_dir() and re.search(r"v\d\.\d\.\d", entry.name):
                    version = entry.name
        if not version:
            error_exit("Failed to detect suitable nRFConnect directory")
        settings['ncs_dir'] = ncs_dir + "/" + version
        print(f"Using ncs version: {version}")
    if not exists(os.path.expandvars(settings['ncs_dir'])):
        error_exit("nRFConnect directory not found")

    # GCC directory, this can be explicitly specified or indirect via nRFConnect
    # directory above. In this case the whole environment is setup via the path
    use_tc = False
    if args.gcc_dir != None:
        settings['gcc_dir'] = args.gcc_dir
    elif not 'gcc_dir' in settings:
        m = re.search(r"(v\d\.\d\.\d)", settings['ncs_dir'])
        if m:
            tc_dir = settings['ncs_dir'] + "/../toolchains/" + m.group(1)
            if exists(tc_dir):
                # Setup and use a complete ncs toolchain
                use_tc = True
                tc_dir = os.path.realpath(tc_dir)
                path = ""
                if 'linux' in sys.platform:
                    path += tc_dir + "/usr/bin:"
                    path += tc_dir + "/usr/local/bin:"
                    path += tc_dir + "/opt/bin:"
                    path += tc_dir + "/opt/nanopb/generator-bin:"
                    path += tc_dir + "opt/zephyr-sdk/arm-zephyr-eabi/bin"
                else:
                    path += tc_dir + ";"
                    path += tc_dir + "\\mingw64\\bin;"
                    path += tc_dir + "\\bin;"
                    path += tc_dir + "\\opt\\bin;"
                    path += tc_dir + "\\opt\\bin\\Scripts;"
                    path += tc_dir + "\\opt/nanopb\\generator-bin;"
                    path += tc_dir + "\\opt\\zephyr-sdk\\arm-zephyr-eabi\\bin;"
                os.environ['PATH'] = path + os.environ['PATH']
        if not use_tc:
            error_exit("Failed to locate GCC directory")
    if not use_tc and not exists(os.path.expandvars(settings['gcc_dir'])):
        error_exit("GCC directory not found")

    if args.ubxlib_dir != None:
        settings['ubxlib_dir'] = args.ubxlib_dir
    elif not 'ubxlib_dir' in settings:
        settings['ubxlib_dir'] = os.path.realpath("ubxlib")
    if not exists(os.path.expandvars(settings['ubxlib_dir'])):
        error_exit("Ubxlib directory not found")

    # The build output root directory
    if args.build_dir != None:
        settings['build_dir'] = args.build_dir
    elif not 'build_dir' in settings:
        settings['build_dir'] = this_dir + "/_build"
    args.build_dir = settings['build_dir'] + "/" + args.example

#--------------------------------------------------------------------



if __name__ == "__main__":

    this_dir = os.path.dirname(os.path.realpath(__file__))
    os.chdir(this_dir)
    check_jlink()
    examples = []
    for entry in os.scandir("."):
        if entry.is_dir() and exists(entry.name + "/src"):
            examples.append(entry.name)
    examples.sort()

    parser = argparse.ArgumentParser()
    parser.add_argument("operation", nargs='+',
                        help="Operation to be performed",
                        )
    parser.add_argument("-e", "--example",
                        help="Name of the example",
                        )
    parser.add_argument("-p", "--pristine",
                        help="Pristine build (rebuild)",
                        action="store_true"
                        )
    parser.add_argument("--no-bootloader",
                        help="Don't use the bootloader",
                        action="store_true"
                        )
    parser.add_argument("--when-changed",
                        help="Only flash when build was triggered",
                        action="store_true"
                        )
    parser.add_argument("-d", "--build-dir",
                        help="Root directory for the build output"
                        )
    parser.add_argument("-n", "--ncs-dir",
                        help="Nrf connect sdk installation directory"
                        )
    parser.add_argument("-t", "--gcc-dir",
                        help="GCC toolchain installation directory"
                        )
    parser.add_argument("-u", "--ubxlib-dir",
                        help="Ubxlib directory"
                        )
    args = parser.parse_args()

    if not args.operation[0] in locals():
        error_exit(f"Invalid operation: \"{args.operation[0]}\"")

    read_settings()
    if args.example == None:
        args.example = "blink"
    else:
        if not args.example in examples:
            error_exit(f"Invalid example \"{args.example}\"\nAvailable: {examples}")
    settings['no_bootloader'] = args.no_bootloader
    check_directories(args)
    set_env()

    # Execute specified operation
    locals()[args.operation[0]](args)

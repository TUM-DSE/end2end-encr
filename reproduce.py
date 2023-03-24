#!/usr/bin/env python3

import os
import signal
from pathlib import Path
from subprocess import Popen, PIPE

BANNER = """
/////////////////////////////////////////////////////////////////
//                                                             //
//                    CMOD Performance Test                    //
//                                                             //
/////////////////////////////////////////////////////////////////
"""

CWD = Path(__file__).parent.resolve().joinpath("bazel-bin", "sw", "device",
                                               "tests", "cmod_perftest_sim_verilator.runfiles", "lowrisc_opentitan")
SIM_PATH = CWD.joinpath("hw", "build.verilator_real", "sim-verilator", "Vchip_sim_tb")
ROM_PATH = CWD.joinpath("sw", "device", "lib", "testing", "test_rom",
                        "test_rom_sim_verilator.39.scr.vmem")
FLASH_PATH = CWD.joinpath("sw", "device", "tests",
                          "cmod_perftest_prog_sim_verilator.fake_prod_key_0.signed.64.scr.vmem")
OTP_PATH = CWD.joinpath("hw", "ip", "otp_ctrl", "data", "img_rma.24.vmem")

RESULTS_FILE = "cmod_perftest_results"

sim_process = None


def handler(signum, frame):
    global sim_process

    if not sim_process is None:
        sim_process.kill()
        print("Simulation process killed.")

    exit(1)


def build_perftest():
    print("Building perftest...\n")
    build_process = Popen(
        args=["./bazelisk.sh", "build", "//sw/device/tests:cmod_perftest_sim_verilator"],
        shell=False
    )

    if build_process.wait():
        exit(1)

    print("\nFinished building process.\n")


def run_perftest():
    global sim_process

    print("Starting simulation in background...")

    sim_process = Popen(
        args=[SIM_PATH,
              f"--meminit=rom,{ROM_PATH}",
              f"--meminit=flash,{FLASH_PATH}",
              f"--meminit=otp,{OTP_PATH}"
              ],
        cwd=CWD,
        shell=False,
        stdout=PIPE,
        stderr=PIPE
    )

    sim_running = False

    while not sim_running:
        line = sim_process.stdout.readline().decode("utf-8").strip()

        if line.find("UART: Created") != -1:
            uart0 = line[14:line.find(" for uart0.")]
        elif line.find("Simulation running") != -1:
            sim_running = True

    print("Simulation is running.\n")

    with open(uart0, "r") as log_file:
        print("Connected to UART0 to print perftest logging.\n")

        results = []

        while sim_running:
            line = log_file.readline().strip()

            if line.find("PASS!") != -1:
                sim_running = False
            else:
                index = line.find("Result: ")

                if index != -1:
                    result = line[(index + 8):]

                    print(result)

                    results.append(result)

        with open(RESULTS_FILE, "w") as output:
            for result in results:
                output.write(f"{result}\n")

    print(f"\nFinished perftest.\nResult was written to: {RESULTS_FILE}")
    sim_process.kill()


if __name__ == "__main__":
    signal.signal(signal.SIGINT, handler)

    print(BANNER)
    build_perftest()
    run_perftest()

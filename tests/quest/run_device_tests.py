"""Run an installed BSML_QUEST_TESTS build through its ADB command interface.
The game must be open on GIF Tests; serve_fixtures.py and adb reverse must be set up.
"""
import argparse
import json
from pathlib import Path
import subprocess
import time

REMOTE = "/sdcard/ModData/com.beatgames.beatsaber/bsml-gif-tests"

class Device:
    def __init__(self, adb, serial, output):
        self.adb = [adb, "-s", serial]
        self.output = output
        output.mkdir(parents=True, exist_ok=True)

    def call(self, *args):
        return subprocess.check_output(self.adb + list(args), text=True, timeout=15, stderr=subprocess.STDOUT)

    def lines(self):
        return self.call("shell", "cat", REMOTE + ".log").splitlines()

    def command(self, command, terminal=None, timeout=15):
        start = len(self.lines())
        path = self.output / "command.txt"
        path.write_text(command, encoding="ascii")
        self.call("push", str(path), REMOTE + ".command")
        end = time.monotonic() + timeout
        while time.monotonic() < end:
            lines = self.lines()[start:]
            if "COMMAND " + command in lines and (not terminal or any(terminal in line for line in lines)):
                if any(line.startswith(("FAIL ", "COMMAND FAILED", "STOP:")) for line in lines):
                    raise RuntimeError("\n".join(lines))
                return lines
            if not self.call("shell", "pidof", "com.beatgames.beatsaber").strip():
                raise RuntimeError("Beat Saber exited during " + command)
            time.sleep(0.5)
        raise TimeoutError("No completion for " + command + "\n" + "\n".join(self.lines()[start:]))

    def resources(self):
        self.command("gc")
        time.sleep(2)
        return self.command("stats", "RESOURCES")[-1]

    def memory(self, name):
        (self.output / (name + ".txt")).write_text(self.call("shell", "dumpsys", "meminfo", "com.beatgames.beatsaber"))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--adb", default="adb")
    parser.add_argument("--device", required=True)
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--output", type=Path, default=Path("build/quest-diagnostics"))
    args = parser.parse_args()
    device = Device(args.adb, args.device, args.output)
    report = {"baseline": device.resources(), "rounds": []}
    print(report["baseline"], flush=True)
    try:
        for i in range(args.rounds):
            print(f"Starting round {i + 1}", flush=True)
            streaming = device.command("streaming", "Streaming finished:", 45)
            offline = device.command("offline", "Offline finished:", 45)
            network = device.command("network", "Network finished:", 60)
            reentry = device.command("reentry", "Reentry finished:", 45)
            resources = device.resources()
            result = {"round": i + 1, "checks": sum(x.startswith("PASS ") for x in streaming + offline + network + reentry),
                      "streaming": streaming[-1],
                      "offline": offline[-1], "network": network[-1], "reentry": reentry[-1], "resources": resources}
            report["rounds"].append(result)
            device.memory(f"memory-round-{i + 1}")
            print(json.dumps(result), flush=True)
    finally:
        (args.output / "repeat-results.json").write_text(json.dumps(report, indent=2))
        device.call("pull", REMOTE + ".log", str(args.output / "in-game-results.log"))

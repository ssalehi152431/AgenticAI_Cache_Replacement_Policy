#!/usr/bin/env python3
import argparse, subprocess, re, json, os, sys
import datetime

from dotenv import load_dotenv
load_dotenv() 

LLC_RE = re.compile(r"LLC TOTAL\s+ACCESS:\s+(\d+)\s+HIT:\s+(\d+)\s+MISS:\s+(\d+)")

def compile_policy(source, config, out_dir):
    pname = os.path.splitext(os.path.basename(source))[0]
    ts = datetime.datetime.now().strftime("%Y%m%d%H%M%S")
    # include config and timestamp in the binary name
    binary_name = f"{pname}-{config}-{ts}"
    binary = os.path.join(out_dir, binary_name)
    libpath = os.path.join("/share/csc591s25/kislam/agent/ChampSim_CRC2/lib", f"{config}.a")
    cmd = [
        "g++", "-Wall", "--std=c++11",
        "-o", binary,
        source, libpath
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        return None, proc.stderr
    return binary, None

def run_trace(binary, trace, warmup, sim):
    cmd = [
        binary,
        "-warmup_instructions", str(warmup),
        "-simulation_instructions", str(sim),
        "-traces", trace
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        return None, proc.stderr
    out = proc.stdout + proc.stderr
    m = LLC_RE.search(out)
    if not m:
        return None, "LLC TOTAL line not found"
    total, hits, _miss = map(int, m.groups())
    rate = hits/total if total else 0.0
    return f"{rate:.4f}", None

# def simulate_main():
#     p = argparse.ArgumentParser(
#         description="Compile a single policy & config, run on traces, emit hit-rates/errors"
#     )
#     p.add_argument("policy", help=".cc or .cpp file under example_policy/")
#     p.add_argument("config", choices=[f"config{i}" for i in range(1,7)])
#     p.add_argument("traces", nargs="+", help=".trace.gz files (full paths ok)")
#     p.add_argument("--warmup", type=int, default=1_000_000)
#     p.add_argument("--sim",    type=int, default=10_000_000)
#     p.add_argument("--outdir", default=".", help="Where to put the compiled binary")
#     args = p.parse_args()
def simulate_main(policy):
    # \"\"\"
    # policy : path to .cc/.cpp
    # other parameters (config, traces, warmup, sim, outdir)
    # are read from environment variables:

    # CONFIG            (e.g. 'config1' … 'config6')
    #   TRACES            (a colon-separated list of .trace.gz paths)
    #   WARMUP_INSTRUCTIONS
    #   SIM_INSTRUCTIONS
    #   OUTDIR
    # \"\"\"
    config = os.getenv("CONFIG", "config1")
    raw_traces = os.getenv("TRACES", "")
    traces = raw_traces.split(":") if raw_traces else []
    warmup = int(os.getenv("WARMUP_INSTRUCTIONS", "1000000"))
    sim    = int(os.getenv("SIM_INSTRUCTIONS",  "10000000"))
    outdir = os.getenv("OUTDIR",               ".")
    results = []
    # Step 1: compile
    # compile step
    
    # source = os.path.join("example_policy", policy)
    source = os.path.join("/share/csc591s25/kislam/agent/ChampSim_CRC2/generated_files", policy)
    print(f"Compiling {source} with config {config} to {outdir}")
    
    binary, compile_err = compile_policy(source, config, outdir)
    if compile_err:
        for trace in traces:
            results.append({
                "trace": os.path.basename(trace),
                "type":  "Error",
                "value": compile_err
            })
        return results    # ← return here instead of sys.exit()

    # Step 2: run on each trace
    for trace in traces:
        val, err = run_trace(binary, trace, warmup, sim)
        if err:
            results.append({
                "trace": os.path.basename(trace),
                "type":  "Error",
                "value": err
            })
        else:
            results.append({
                "trace": os.path.basename(trace),
                "type":  "HitRate",
                "value": val
            })

    return results

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: simulate-champsim.py <policy_file.cc>", file=sys.stderr)
        sys.exit(1)
    policy_file = sys.argv[1]
    results = simulate_main(policy_file)
    print(json.dumps(results, indent=2))

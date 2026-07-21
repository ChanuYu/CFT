# CFT: Compression for Tiering

CFT (Compression for Tiering) is a Flash Translation Layer (FTL) architecture
that selectively compresses highly compressible cold data to reclaim internal
space, and provisions the reclaimed space as a single-level-cell (SLC) tier for
hot data. Unlike conventional hybrid SSDs that borrow SLC capacity from unused
free blocks—so the SLC region must shrink as space utilization grows—CFT
provisions the SLC tier from compression-reclaimed internal space, so the tier
can be retained as long as sufficient live reclaimed-space backing exists. By
keeping hot data resident in the SLC tier rather than treating it as a transient
write buffer, CFT can improve both read and write performance when the workload
provides sufficiently compressible cold data and idle time.

This repository contains the SSD-simulator implementation of CFT (built on
[SSDsim](https://github.com/huaicheng/ssdsim)) and the traces used in the
evaluation.

## Repository layout

```
cft/          CFT: compression-provisioned SLC tier, implemented on SSDsim
dcft/         DCFT: CFT added on top of a utilization-coupled SLC-buffer baseline
trace/        Real-world block-I/O traces used in the evaluation
  web         Web / SNS (video streaming and browsing) workload
  rsrch       Document authoring and web-search workload
```

Both `cft/` and `dcft/` are built on [SSDsim](https://github.com/huaicheng/ssdsim),
an SSD simulator; each contains the modified SSDsim source (`src/`), a
`Makefile`, and a `page.parameters` device-configuration file. The unmodified
SSDsim behavior serves as the baseline (referred to as "Vanilla" in the paper).

## Build

```
cd cft      # or: cd dcft
make        # produces the `main` executable
```

Requires a C compiler (gcc/clang) and `make`.

## Run

```
./main <trace_name> [prefill_name]
```

The simulator reads the device configuration from `page.parameters` and replays
the trace given by `<trace_name>` (looked up under `trace/`). Per-run statistics
(latency, WAF, SLC-tier usage, etc.) are written to the output/statistics files.

An optional second argument, `prefill_name`, gives a trace that is replayed
first to age the device (fill it to a target utilization) before the main trace
is measured; statistics are collected only over the main trace. A prefill is
required for the real-world traces here: without it the device never reaches the
utilization at which compression is triggered, and no SLC tier is created.

To reproduce the paper's real-world results, run each trace with its prefill:

```
./main web   web-prefill     # web workload
./main rsrch rsrch-prefill   # rsrch workload
```

Trace record format (one request per line):

```
<arrival_time_ns> <device> <logical_sector> <size_sectors> <op> <pred_ratio> <real_ratio>
```

where `op` is `1` for read and `0` for write, and `pred_ratio` / `real_ratio`
are the predicted and actual compression ratios of the request's data
(ratio = compressed size / original size; smaller is more compressible).

## Traces

| Trace | Prefill | Description |
|-------|---------|-------------|
| `web`   | `web-prefill`   | Video-streaming and web-browsing activity (read-dominant, large read I/O). |
| `rsrch` | `rsrch-prefill` | Document authoring and web search (mixed read/write). |

These traces exercise CFT under real-world access patterns with distinct
hot/cold behavior. Each is paired with a prefill trace that must be supplied as
the second argument (see [Run](#run)) to age the device before measurement.

## Citation

If you use this code or the traces, please cite the accompanying paper
(C. Yu, J. Kim, and E. Seo, "Compression for Tiering: Enabling High-Tier Flash
Storage for Hot Data via Cold Data Compression").

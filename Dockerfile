FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update; apt install -y git make binutils-mips-linux-gnu cpp-mips-linux-gnu python3 python3-pip python3-venv
# pc_port differential harness (docs/ai_context/HARNESS_REGIMES.md):
#   clang    - a large number of pc_port/tests/run_*.sh use clang, not gcc,
#              for their UBSan regime; without it those regimes cannot run
#              at all and the suite dies on "clang: command not found".
#   ripgrep  - every runner uses `rg` as its assertion matcher.
#   libubsan1 - the UBSan runtime itself. Pulled in by gcc here, but named
#              explicitly because its absence is what silently cost this
#              project a third of the O0/O2/UBSan verification bar.
RUN apt install -y clang ripgrep libubsan1

COPY requirements.txt /tmp/requirements.txt
RUN python3 -m venv /.venv && . /.venv/bin/activate && python3 -m pip install -r /tmp/requirements.txt
# NB: Don't rename this. Gears depends on this to figure out where the project files
# are located. See find_base_path() in tools/gears/src/file_system.rs
WORKDIR /xenogears-decomp

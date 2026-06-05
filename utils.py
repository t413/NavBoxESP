#! /usr/bin/env python
import os, sys, subprocess
import pathlib, requests

PROJ_ROOT = pathlib.Path(os.getcwd())
TILE_ROOT = PROJ_ROOT / "tiles"
TILE_GENDIR = PROJ_ROOT / "lib" / "staticfiles"
BASE_TILES = [(z, x, y) for z in range(3) for x in range(1 << z) for y in range(1 << z)] + [
  (4, 2, 6), #CA
  (4, 4, 6), #EST
  (4, 8, 5), #EU
]
TILE_BASE_URL = "https://t413.com/map/tiles"

def shellCmd(cmd):
  return subprocess.check_output(cmd.split(' ')).strip().decode("utf-8")

def getDescribe():
  return shellCmd("git describe --long --tags --always --dirty")
def getGitDate():
  return shellCmd("git log -1 --date=format:%Y%m%d --format=%ad")
def getVersion():
  try:
    return getDescribe().replace("-dirty", ".d") + "-" + str(getGitDate())
  except Exception as e:
    return os.path.basename(os.getcwd())
def prettyPrint():
  try: #optional colorful output
    from colorama import Fore, Back, Style
    print(Back.YELLOW + Fore.BLACK + " git version " + Back.BLACK + Fore.YELLOW + " " + getVersion() + " " + Style.RESET_ALL)
  except Exception:
    print("git version " + getVersion())

def ensure_base_tiles():
  # first add a couple z3, z4, and z5 tiles in populated areas:
  for z, x, y in BASE_TILES:
    target = TILE_ROOT / str(z) / str(x) / f"{y}.png"
    if target.exists():
      continue
    target.parent.mkdir(parents=True, exist_ok=True)
    url = f"{TILE_BASE_URL}/{z}/{x}/{y}.png"
    print(f" -> Fetching base tile {z}/{x}/{y} -> {target}")
    r = requests.get(url, timeout=10)
    r.raise_for_status()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(r.content)
  print(f" - Have {len(BASE_TILES)} base tiles in {TILE_ROOT}")

def write_basetiles_source(out_dir):
  out_dir = pathlib.Path(out_dir)
  out_dir.mkdir(parents=True, exist_ok=True)

  source_path = out_dir / "basetiles.cpp"

  with source_path.open("w", newline="\n") as f:
    f.write('#include "basetiles.h"\n\n')

    for z, x, y in BASE_TILES:
      path = TILE_ROOT / str(z) / str(x) / f"{y}.png"
      data = path.read_bytes()
      name = f"tile_{z}_{x}_{y}"
      f.write(f"static const uint8_t {name}[] = {{\n")

      for i in range(0, len(data), 16):
        line = ", ".join(f"0x{b:02x}" for b in data[i:i+16])
        f.write("    " + line + ",\n")

      f.write("};\n\n")

    f.write("const StaticTile kBaseTiles[] = {\n")
    for z, x, y in BASE_TILES:
      name = f"tile_{z}_{x}_{y}"
      f.write(f"    {{ {z}, {x}, {y}, {name}, sizeof({name}) }},\n")
    f.write("};\n\n")
    f.write(f"const uint32_t kBaseTileCount = {len(BASE_TILES)};\n")
  print(f" - wrote tile data to {source_path} / cpp")

arg = sys.argv[1] if len(sys.argv) > 1 else ""

if arg == "version":
  prettyPrint()
elif arg == "simple":
  print(getVersion())

else:
  prettyPrint()

  try: #if running inside platformio
    Import("env")
    # print(env.Dump()) # <- can use this to see what's available
    bpath = os.path.join(env.subst("$BUILD_DIR"), "generated")
    print(" - version injection to " + bpath)

    if not os.path.exists(bpath): os.makedirs(bpath)
    with open(os.path.join(bpath, "version.cpp"), 'w+') as ofile:
      ofile.write("const char* GIT_VERSION(\"" + getVersion() + "\");" + os.linesep)

    ensure_base_tiles()
    write_basetiles_source(TILE_GENDIR)

    env.BuildSources(os.path.join(bpath, "build"), bpath)
  except NameError:
    pass

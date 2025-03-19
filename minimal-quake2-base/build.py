import os
import subprocess
import argparse
import shutil
import shlex
from pathlib import Path

yquake2_url = "https://github.com/yquake2/yquake2.git"
yquake2_commit = "d2efa1c9af5ef649a91cf5de4bfa2370c03f69f2"
yquake2_dir = Path("yquake2")

yquake2_ref_vk_url = "https://github.com/yquake2/ref_vk"
yquake2_ref_vk_commit = "21bde3c4bb3ab3af00d41c2fd85b86c0a021732f"
yquake2_ref_vk_dir = Path("ref_vk")

game_c_dir = Path("game-c")

base_dir = Path("base")

tmp_dir = Path("tmp")
pak0_dir = tmp_dir / "pak0"

release_dir = Path("release")

build_odin = False


def clone_yquake2():
    if os.path.exists(yquake2_dir):
        print(f"Directory {yquake2_dir} already exists. Skipping clone.")
        return

    subprocess.run(["git", "clone", yquake2_url, yquake2_dir], check=True)
    subprocess.run(["git", "checkout", yquake2_commit], check=True, cwd=yquake2_dir)


def clone_yquake2_ref_vk():
    if os.path.exists(yquake2_ref_vk_dir):
        print(f"Directory {yquake2_ref_vk_dir} already exists. Skipping clone.")
        return

    subprocess.run(["git", "clone", yquake2_ref_vk_url, yquake2_ref_vk_dir], check=True)
    subprocess.run(
        ["git", "checkout", yquake2_ref_vk_commit], check=True, cwd=yquake2_ref_vk_dir
    )


def clone():
    clone_yquake2()
    clone_yquake2_ref_vk()


def build_yquake2():
    print("Building yquake2")
    subprocess.run(["make"], check=True, cwd=yquake2_dir)


def build_yquake2_ref_vk():
    print("Building yquake2 ref_vk")
    subprocess.run(["make"], check=True, cwd=yquake2_ref_vk_dir)


def build_game_odin():
    print("Building game-odin")
    (release_dir / "baseq2").mkdir(parents=True, exist_ok=True)
    cmd = shlex.split(
        "odin build ./game-odin -build-mode:dll -out:./release/baseq2/game.dylib"
    )
    subprocess.run(cmd, check=True)


def build_game_c():
    print("Building game-c")
    (release_dir / "baseq2").mkdir(parents=True, exist_ok=True)
    subprocess.run(["make", "DEBUG=0"], check=True, cwd=game_c_dir)
    game_lib_src = game_c_dir / "release" / "game.dylib"
    game_lib_dst = release_dir / "baseq2" / "game.dylib"
    shutil.copy2(game_lib_src, game_lib_dst)


def build_maps():
    maps_dir = base_dir / "maps"
    map_files = list(maps_dir.glob("*.map"))

    for map_file in map_files:
        map_name = map_file.stem
        print(f"Building map: {map_name}")
        subprocess.run(["qbsp", "-q2bsp", f"{map_name}.map"], check=True, cwd=maps_dir)
        subprocess.run(["vis", f"{map_name}.bsp"], check=True, cwd=maps_dir)
        subprocess.run(["light", f"{map_name}.bsp"], check=True, cwd=maps_dir)


def copy_directory_recursively(src, dst, **kwargs):
    if not dst.exists():
        dst.mkdir(parents=True, exist_ok=True)

    ignore_extensions = kwargs.get("ignore_extensions", [])

    for item in src.iterdir():
        if item.is_dir():
            copy_directory_recursively(item, dst / item.name, **kwargs)
        else:
            if item.suffix not in ignore_extensions:
                print(f"Copying {item} to {dst / item.name}")
                shutil.copy2(item, dst / item.name)


def copy_file_maintaining_path(
    source_base: Path, relative_path: Path, target_base: Path
):
    if "*" in str(relative_path):
        parent_dir = relative_path.parent
        pattern = relative_path.name

        source_dir = source_base / parent_dir

        matching_files = []
        for file in source_dir.glob(pattern):
            matching_files.append(file)
    else:
        matching_files = [source_base / relative_path]

    for source_file in matching_files:
        rel_file_path = source_file.relative_to(source_base)
        target_file = target_base / rel_file_path

        target_dir = target_file.parent
        if not target_dir.exists():
            target_dir.mkdir(parents=True, exist_ok=True)

        print(f"Copying {source_file} to {target_file}")
        shutil.copy2(source_file, target_file)


def copy_files():
    print("Copying files to release directory")

    if not release_dir.exists():
        release_dir.mkdir()
        print(f"Created {release_dir} directory")

    files = [
        yquake2_dir / "release" / "q2ded",
        yquake2_dir / "release" / "quake2",
        yquake2_ref_vk_dir / "release" / "ref_vk.dylib",
    ]

    for file in files:
        print(f"Copying {file} to {release_dir}")
        shutil.copy2(file, release_dir)

    baseq2_dir = release_dir / "baseq2"
    baseq2_dir.mkdir(parents=True, exist_ok=True)

    copy_directory_recursively(
        base_dir,
        baseq2_dir,
        ignore_extensions=[
            ".aseprite",
            ".map",
            ".log",
            ".prt",
            ".vis",
            ".json",
        ],
    )

    # NOTE: this is in here so i can get a running game
    # the intent is to have no files copied from the pak0 file
    # so the release is all in-repo assets, therefore no
    # copyright infringement!

    paths = [
        "pics/colormap.pcx",
        "pics/conchars.pcx",
        "pics/conback.pcx",
        "pics/ch1.pcx",
        "pics/pause.pcx",
        "pics/m_main_*.pcx",
        "pics/quit.pcx",
    ]

    for path in paths:
        copy_file_maintaining_path(pak0_dir, Path(path), baseq2_dir)

    print("Copying files to release directory completed")


def build():
    build_yquake2()
    build_yquake2_ref_vk()
    build_maps()
    copy_files()

    if build_odin:
        build_game_odin()
    else:
        build_game_c()


def run():
    env = os.environ.copy()
    env["DYLD_LIBRARY_PATH"] = "/opt/homebrew/opt/molten-vk/lib"
    subprocess.run(["./quake2"], check=True, cwd=release_dir, env=env)


def all():
    clone()
    build()
    run()


def setup_trenchbroom():
    games_dir = Path(
        os.path.expanduser("~/Library/Application Support/TrenchBroom/games/")
    )

    if not games_dir.exists():
        print("TrenchBroom games directory not found, not setting up Trenchbroom")
        return

    games_dir = games_dir / "MinimalQuake2Base"

    games_dir.mkdir(parents=True, exist_ok=True)

    subprocess.run(
        [
            "cp",
            "-r",
            "./trenchbroom-config/",
            games_dir,
        ],
        check=True,
    )


def loc_metrics():
    output_file = "game-c-loc.txt"

    with open(output_file, "w") as f:
        subprocess.run(["tokei", game_c_dir], stdout=f, check=True)

    print(f"Lines of code metrics written to {output_file}")


def main():
    parser = argparse.ArgumentParser(description="Build tools for minimal-quake2-base")
    subparsers = parser.add_subparsers(dest="command", help="Commands")

    subparsers.add_parser("clone", help="Clone repositories")
    subparsers.add_parser("build", help="Build targets")
    subparsers.add_parser("build-maps", help="Build maps")
    subparsers.add_parser("all", help="Do everything")
    subparsers.add_parser("run", help="Run the game")
    subparsers.add_parser("copy", help="Copy files")
    subparsers.add_parser("copy-and-run", help="Copy files and run the game")
    subparsers.add_parser("setup-trenchbroom", help="Setup Trenchbroom")
    subparsers.add_parser(
        "loc-metrics", help="Get metrics on how much code is in game-c"
    )

    args = parser.parse_args()

    if args.command == "clone":
        clone()
    elif args.command == "build":
        build()
    elif args.command == "build-maps":
        build_maps()
    elif args.command == "all":
        all()
    elif args.command == "run":
        run()
    elif args.command == "copy":
        copy_files()
    elif args.command == "copy-and-run":
        copy_files()
        run()
    elif args.command == "setup-trenchbroom":
        setup_trenchbroom()
    elif args.command == "loc-metrics":
        loc_metrics()
    elif not args.command:
        parser.print_help()


if __name__ == "__main__":
    main()

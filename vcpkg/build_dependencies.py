# Script to build all of NVGT's dependencies with vcpkg, setting them up for NVGT builds and optionally creating redistributable archives for the dependencies.
# This only supports the triplets that we currently use as well as IOS.
# NVGT - NonVisual Gaming Toolkit (https://nvgt.dev)
# Copyright (c) 2022-2025 Sam Tupy
# license: zlib
import os
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys

vcpkg_path = Path(__file__, "..", "bin", "vcpkg" if sys.platform != "win32" else "vcpkg.exe").resolve()
vcpkg_installed_path = Path(__file__, "..", "vcpkg_installed").resolve()
repo_path = Path(__file__).parents[1]

def bootstrap_vcpkg():
	if vcpkg_path.exists() and vcpkg_path.is_file():
		return
	if sys.platform == "win32":
		subprocess.check_output(vcpkg_path.parent / "bootstrap-vcpkg.bat")
	else:
		subprocess.check_output(vcpkg_path.parent / "bootstrap-vcpkg.sh")
def prefetch_github_api_downloads():
	# The pcre2 port asks the GitHub API (unauthenticated) which sljit commit its release tag pins. CI runners share IPs and often hit the 60 requests/hour limit, failing with HTTP 403. When a GITHUB_TOKEN is available, fetch that file ourselves with authentication and drop it in vcpkg's downloads folder; vcpkg still verifies its SHA512 and simply uses it instead of downloading.
	token = os.environ.get("GITHUB_TOKEN")
	if not token: return
	import json, urllib.request
	try: version = json.loads((vcpkg_path.parent / "ports" / "pcre2" / "vcpkg.json").read_text())["version"]
	except (OSError, KeyError, ValueError): return
	ref = f"pcre2-{version}"
	downloads = Path(os.environ.get("VCPKG_DOWNLOADS", vcpkg_path.parent / "downloads"))
	dest = downloads / f"pcre2-{ref}-sljit-gitlink.json"
	if dest.exists(): return
	req = urllib.request.Request(f"https://api.github.com/repos/PCRE2Project/pcre2/contents/deps/sljit?ref={ref}", headers = {"Authorization": f"Bearer {token}", "User-Agent": "curl/8"}) # GitHub only pretty-prints the JSON (which the port's SHA512 is computed over) for curl user agents.
	try:
		with urllib.request.urlopen(req, timeout = 30) as r: data = r.read()
	except OSError as e:
		print(f"warning: could not prefetch the pcre2 sljit gitlink ({e}), vcpkg will try itself")
		return
	# vcpkg fails outright on a cached download with the wrong hash rather than refetching it, so only keep the file if it matches what the port expects.
	import re
	try: expected = re.search(r"sljit_gitlink_hash SHA512 ([0-9a-f]{128})", (vcpkg_path.parent / "ports" / "pcre2" / "portfile.cmake").read_text())
	except OSError: expected = None
	if not expected or hashlib.sha512(data).hexdigest() != expected.group(1):
		print("warning: prefetched pcre2 sljit gitlink does not match the port's SHA512, leaving the download to vcpkg")
		return
	downloads.mkdir(parents = True, exist_ok = True)
	dest.write_bytes(data)
def build(triplet = "", do_archive = False, out_dir = ""):
	if not triplet:
		# Try to determine, logic probably could be improved
		if sys.platform == "win32": triplet = "x64-windows"
		elif sys.platform == "darwin": triplet = "arm64-osx"
		elif sys.platform == "linux":
                    if(os.uname().machine.__contains__("x86")):
                        triplet = "x64-linux"
                    elif(os.uname().machine.__contains__("aarch64")):
                        triplet = "arm64-linux"
		else: sys.exit("unable to determine platform, please pass a triplet explicitly.")
	bootstrap_vcpkg()
	prefetch_github_api_downloads()
	try: subprocess.run([vcpkg_path, "install", "--triplet", triplet, "--x-manifest-root", vcpkg_path.parents[1]], check=True)
	except subprocess.CalledProcessError as cpe: sys.exit(f"Building packages for {triplet} failed with error code {cpe.returncode}.")
	dev_basename = ""
	if "-windows" in triplet: dev_basename = "windev"
	elif "-osx" in triplet: dev_basename = "macosdev"
	elif "-linux" in triplet: dev_basename = "lindev"
	elif "-android" in triplet: dev_basename = "droidev"
	elif "-ios" in triplet: dev_basename = "iosdev"
	if not out_dir: out_dir = repo_path / dev_basename
	else: out_dir = Path(out_dir)
	out_dir.mkdir(parents=True, exist_ok=True)
	if (vcpkg_installed_path / triplet / "bin").exists(): shutil.copytree(vcpkg_installed_path / triplet / "bin", out_dir / "bin", dirs_exist_ok = True)
	if (vcpkg_installed_path / triplet / "debug" / "bin").exists(): shutil.copytree(vcpkg_installed_path / triplet / "debug" / "bin", out_dir / "debug" / "bin", dirs_exist_ok = True)
	shutil.copytree(vcpkg_installed_path / triplet / "debug" / "lib", out_dir / "debug" / "lib", dirs_exist_ok = True)
	shutil.copytree(vcpkg_installed_path / triplet / "include", out_dir / "include", dirs_exist_ok = True)
	shutil.copytree(vcpkg_installed_path / triplet / "lib", out_dir / "lib", dirs_exist_ok = True)
	fix_debug(out_dir)
	if triplet == "arm64-osx": macos_fat_binaries(out_dir)
	elif triplet == "x64-windows": windows_lib_rename(out_dir)
	if triplet.endswith("osx") or triplet.endswith("linux"): remove_duplicates(out_dir)
	try:
		shutil.rmtree(out_dir / "lib" / "cmake")
		shutil.rmtree(out_dir / "lib" / "pkgconfig")
		shutil.rmtree(out_dir / "debug" / "lib" / "cmake")
		shutil.rmtree(out_dir / "debug" / "lib" / "pkgconfig")
	except FileNotFoundError: pass
	if do_archive:
		shutil.make_archive(out_dir, format = "zip", root_dir = out_dir)
		with out_dir.with_suffix(".zip").open("rb") as f, out_dir.with_suffix(".zip.blake2b").open("w") as hf:
			h = hashlib.blake2b()
			h.update(f.read())
			hf.write(h.hexdigest())
def macos_fat_binaries(out_dir):
	"""We must manually build Angelscript, Steam Audio, libffi and openssl for x64 as well and then run lipo on both of our builds to create universal binaries for them. This is meant to be run after the macosdev directory is created. We basically need to do this for libraries that contain assembly instructions, because the Clang fat builds of such libraries either fail or break down when running NVGT."""
	try: subprocess.check_output([vcpkg_path, "install", "--classic", "--triplet", "x64-osx", "--overlay-ports=" + str(Path(__file__).parent / "ports"), "--overlay-triplets=" + str(Path(__file__).parent / "triplets"), "angelscript", "angelscript-nc", "libffi", "openssl", "steam-audio"])
	except subprocess.CalledProcessError as cpe: sys.exit(f"Building angelscript libffi and openssl for x64-osx failed with error code {cpe.returncode}.\n{cpe.output.decode()}")
	for f in ["libangelscript.a", "libangelscript_nc.a", "libcrypto.a", "libffi.a", "libphonon.a", "libssl.a"]:
		(out_dir / "debug" / "lib" / f).unlink()
		(out_dir / "lib" / f).unlink()
		subprocess.check_output(["lipo", "-create", vcpkg_path.parent / "installed" / "x64-osx" / "debug" / "lib" / f, vcpkg_installed_path / "arm64-osx" / "debug" / "lib" / f, "-output", out_dir / "debug" / "lib" / f])
		subprocess.check_output(["lipo", "-create", vcpkg_path.parent / "installed" / "x64-osx" / "lib" / f, vcpkg_installed_path / "arm64-osx" / "lib" / f, "-output", out_dir / "lib" / f])
def fix_debug(out_dir):
	"""Several debug libraries frustratingly have different filenames to their release counterparts, which just makes build scripts more complicated. We'll just fix it here. Usually a d is just tacked on to the end of the filename which we'll get rid of as well as a hifen that sometimes appears."""
	excludes = ["reactphysics3d", "zstd"]
	for f in (out_dir / "debug" / "lib").iterdir():
		if not f.stem.lower().endswith("d"): continue
		exclude = f.stem if not f.stem.startswith("lib") else f.stem[3:]
		if exclude in excludes: continue
		count = 1 if not f.stem.lower().endswith("-d") else 2
		f.replace(f.with_stem(f.stem[:-count]))
def windows_lib_rename(out_dir):
	"""Sometimes windows libraries get built with annoying names that complicate build scripts. We have to handle them somewhere, may as well be here."""
	renames = [
		("libcrypto", "crypto"),
		("libcurl", "curl"),
		("libexpatMT", "expat"),
		("libexpatdMT", "expat"),
		("libssl", "ssl"),
		("pocoCryptomt", "pocoCrypto"),
		("pocoEncodingsmt", "pocoEncodings"),
		("pocoFoundationmt", "pocoFoundation"),
		("pocoJSONmt", "pocoJSON"),
		("pocoJWTmt", "pocoJWT"),
		("pocoMongoDBmt", "pocoMongoDB"),
		("pocoNetmt", "pocoNet"),
		("pocoNetSSLmt", "pocoNetSSL"),
		("pocoPrometheusmt", "pocoPrometheus"),
		("pocoRedismt", "pocoRedis"),
		("pocoSevenZipmt", "pocoSevenZip"),
		("pocoUtilmt", "pocoUtil"),
		("pocoXMLmt", "pocoXML"),
		("pocoZipmt", "pocoZip"),
		("SDL3-static", "SDL3"),
		("SDL3_image-static", "SDL3_image"),
		("SDL3_ttf-static", "SDL3_ttf"),
		("utf8proc_static", "utf8proc"),
		("zlib", "z")
	] # end renames list
	for lib in ["debug/lib", "lib"]:
		for r in renames:
			if (out_dir / lib / (r[0] + ".lib")).exists(): (out_dir / lib / (r[0] + ".lib")).replace(out_dir / lib / (r[1] + ".lib"))
def remove_duplicates(out_dir):
	"""A couple libraries on Linux and MacOS might have created duplicate versions of themselves because of symlinks, lets get rid of them. We'll also perform install_name_tool fixes on MacOS."""
	for lib in ["libarchive", "libgit2"]:
		for libdir in ["debug/lib", "lib"]:
			versions = list((out_dir / libdir).glob(lib + "*"))
			if len(versions) < 2: continue
			versions.sort(key = lambda v: len(v.name))
			for v in versions[1:]: v.unlink()
			if sys.platform == "darwin": subprocess.check_output(["install_name_tool", "-id", f"@rpath/{versions[0].name}", str(versions[0])])

if __name__ == "__main__":
	triplets = []
	do_archive = False
	if len(sys.argv) > 1:
		for arg in sys.argv[1:]:
			if arg == "--archive": do_archive = True
			else: triplets.append(arg)
	if len(triplets) < 1: triplets.append("")
	for t in triplets: build(t, do_archive)

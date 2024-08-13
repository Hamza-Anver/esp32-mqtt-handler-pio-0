import subprocess
import datetime
import os

def get_git_commit_hash():
    try:
        return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"]).strip().decode("utf-8")
    except:
        return "unknown"

def get_git_commit_count():
    try:
        return subprocess.check_output(["git", "rev-list", "--count", "HEAD"]).strip().decode("utf-8")
    except:
        return "0"

def get_git_branch():
    try:
        return subprocess.check_output(["git", "rev-parse", "--abbrev-ref", "HEAD"]).strip().decode("utf-8")
    except:
        return "unknown"

def generate_version_header(source_dir):
    commit_hash = get_git_commit_hash()
    commit_count = get_git_commit_count()
    branch = get_git_branch()
    build_date = datetime.datetime.now().strftime("%Y-%m-%d")
    build_time = datetime.datetime.now().strftime("%H:%M:%S")

    version_header_content = f"""
#ifndef VERSION_H
#define VERSION_H

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH {commit_count}
#define VERSION_GIT_HASH "{commit_hash}"
#define VERSION_GIT_BRANCH "{branch}"
#define VERSION_BUILD_DATE "{build_date}"
#define VERSION_BUILD_TIME "{build_time}"

#define VERSION_STRING "v1.0.{commit_count} ({branch}-{commit_hash}) {build_date} {build_time}"

#endif // VERSION_H
"""

    with open(source_dir + "/include/version.h", "w") as f:
        f.write(version_header_content)

def before_build(env):
    generate_version_header(env['PROJECT_SRC_DIR'])

Import("env")
env.AddPreAction("build", before_build)

"""Prepare and publish a GameAnalytics C++ SDK release.

1. Open the version bump PR:

       python release.py prepare 5.4.3 --fixed "**Linux** — ..." --added "..."

   Sets GA_VERSION_STR in source/gameanalytics/GACommon.h, adds the changes to
   CHANGELOG.md, commits them on release/v5.4.3, pushes it and opens a PR against main.
   Without change flags, an existing "## 5.4.3" or "## Unreleased" section is used as is.

2. Once the PR is merged:

       python release.py publish 5.4.3

   Tags the merge commit as v5.4.3, pushes the tag and triggers the
   "Create Release" workflow through gh_create_release.py.
"""
import argparse
import json
import os
import re
import subprocess
import sys

from gh_create_release import check_gh_cli_installed, trigger_workflow

ROOT = os.path.dirname(os.path.abspath(__file__))
BASE_BRANCH = "main"
CHANGELOG = "CHANGELOG.md"
VERSION_HEADER = "source/gameanalytics/GACommon.h"
VERSION_PATTERN = re.compile(r'(GA_VERSION_STR\s*=\s*"cpp )(\d+\.\d+\.\d+)(")')
CHANGELOG_CATEGORIES = ["Added", "Changed", "Fixed", "Removed"]


def run(*command, check=True):
    """Run a command from the repo root and return its stripped stdout, or None if it fails and check is False."""
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    if result.returncode != 0:
        if not check:
            return None
        sys.exit(f"Command failed: {' '.join(command)}\n{result.stderr.strip()}")
    return result.stdout.strip()


def read_file(path):
    # newline='' keeps the file's own line endings (GACommon.h is CRLF)
    with open(os.path.join(ROOT, path), encoding="utf-8", newline="") as f:
        return f.read()


def write_file(path, text):
    with open(os.path.join(ROOT, path), "w", encoding="utf-8", newline="") as f:
        f.write(text)


def parse_version(version):
    """Accept '1.2.3' or 'v1.2.3' and return '1.2.3'."""
    match = re.fullmatch(r"v?(\d+\.\d+\.\d+)", version)
    if not match:
        sys.exit(f"Invalid version '{version}'. Please use the format '1.0.0'.")
    return match.group(1)


def header_version(header):
    match = VERSION_PATTERN.search(header)
    if not match:
        sys.exit(f"GA_VERSION_STR not found in {VERSION_HEADER}.")
    return match.group(2)


def local_tag_commit(tag):
    return run("git", "rev-parse", "-q", "--verify", f"refs/tags/{tag}^{{commit}}", check=False)


def remote_tag_commit(tag):
    """Return the commit the tag points to on origin, or None if it doesn't exist there."""
    refs = {}
    for line in run("git", "ls-remote", "origin", f"refs/tags/{tag}", f"refs/tags/{tag}^{{}}").splitlines():
        sha, ref = line.split("\t")
        refs[ref] = sha
    # Annotated tags are listed twice; the ^{} entry is the commit they point to
    return refs.get(f"refs/tags/{tag}^{{}}") or refs.get(f"refs/tags/{tag}")


def heading_level(line):
    match = re.match(r"(#+)\s", line)
    return len(match.group(1)) if match else None


def block_end(lines, start, max_level):
    """Index of the first heading after start at max_level or above, or len(lines)."""
    for i in range(start + 1, len(lines)):
        level = heading_level(lines[i])
        if level and level <= max_level:
            return i
    return len(lines)


def trim_blank(lines, start, end):
    """Move end back over trailing blank lines, stopping just after start."""
    while end > start + 1 and not lines[end - 1].strip():
        end -= 1
    return end


def update_changelog(text, version, changes):
    """Add changes to the '## <version>' section and return (new text, section body).

    An existing '## <version>' section is reused and '## Unreleased' is renamed;
    otherwise a new section is inserted above the latest release.
    """
    lines = text.split("\n")

    def find_section(title):
        return next((i for i, line in enumerate(lines)
                     if heading_level(line) == 2 and line[3:].strip().lower() == title), None)

    start = find_section(version)
    if start is None:
        start = find_section("unreleased")
        if start is not None:
            lines[start] = f"## {version}"
    if start is None:
        if not any(changes.values()):
            sys.exit(f"No changes given and {CHANGELOG} has no '## {version}' or '## Unreleased' section.")
        start = next((i for i, line in enumerate(lines) if heading_level(line) == 2), len(lines))
        lines[start:start] = [f"## {version}", ""]

    for category in CHANGELOG_CATEGORIES:
        entries = [f"- {entry}" for entry in changes[category]]
        if not entries:
            continue
        section_end = block_end(lines, start, 2)
        sub = next((i for i in range(start + 1, section_end) if lines[i].strip() == f"### {category}"), None)
        if sub is None:
            insert_at = trim_blank(lines, start, section_end)
            entries = ["", f"### {category}", ""] + entries
        else:
            insert_at = trim_blank(lines, sub, block_end(lines, sub, 3))
        lines[insert_at:insert_at] = entries

    section = "\n".join(lines[start + 1:block_end(lines, start, 2)]).strip()
    return "\n".join(lines), section


def pr_body(version, notes):
    return (f"Version bump ahead of the `v{version}` release.\n\n"
            f"- `GA_VERSION_STR` → `cpp {version}` (`{VERSION_HEADER}`)\n"
            f"- `{CHANGELOG}`: `## {version}`\n\n"
            f"{notes}\n\n"
            f"Once merged, tag and release with `python release.py publish {version}`.\n")


def prepare(args):
    """Bump the version, update the changelog and open the release PR."""
    version = parse_version(args.version)
    tag = f"v{version}"
    branch = f"release/{tag}"

    run("gh", "auth", "status")
    run("git", "fetch", "origin", BASE_BRANCH)
    if run("git", "rev-parse", "HEAD") != run("git", "rev-parse", f"origin/{BASE_BRANCH}"):
        sys.exit(f"HEAD is not at origin/{BASE_BRANCH}. Check out {BASE_BRANCH} and pull first.")
    dirty = set(run("git", "diff", "--name-only", "HEAD").splitlines()) - {CHANGELOG}
    if dirty:
        sys.exit(f"Uncommitted changes outside {CHANGELOG}:\n  " + "\n  ".join(sorted(dirty)))
    if local_tag_commit(tag) or remote_tag_commit(tag):
        sys.exit(f"Tag {tag} already exists.")
    if run("git", "rev-parse", "-q", "--verify", f"refs/heads/{branch}", check=False) \
            or run("git", "ls-remote", "--heads", "origin", branch):
        sys.exit(f"Branch {branch} already exists.")

    header = read_file(VERSION_HEADER)
    current = header_version(header)
    if tuple(map(int, version.split("."))) < tuple(map(int, current.split("."))):
        sys.exit(f"Version {version} is lower than the current {current}.")
    header = VERSION_PATTERN.sub(rf"\g<1>{version}\g<3>", header, count=1)
    changes = {category: getattr(args, category.lower()) for category in CHANGELOG_CATEGORIES}
    changelog, notes = update_changelog(read_file(CHANGELOG), version, changes)

    print(f"Creating {branch} ({current} -> {version})")
    run("git", "checkout", "-b", branch)
    write_file(VERSION_HEADER, header)
    write_file(CHANGELOG, changelog)
    run("git", "commit", "-m", f"Bump version to {version}", "--", VERSION_HEADER, CHANGELOG)
    run("git", "push", "-u", "origin", branch)
    run("git", "checkout", "-")

    url = run("gh", "pr", "create", "--base", BASE_BRANCH, "--head", branch,
              "--title", f"Bump version to {version}", "--label", "ignore-for-release",
              "--body", pr_body(version, notes))
    print(f"Opened {url}")
    print(f"Once it is merged, run: python release.py publish {version}")


def publish(args):
    """Tag the merged release PR and trigger the Create Release workflow."""
    version = parse_version(args.version)
    tag = f"v{version}"
    branch = f"release/{tag}"

    pr = json.loads(run("gh", "pr", "view", branch, "--json", "state,mergeCommit,url"))
    if pr["state"] != "MERGED":
        sys.exit(f"{pr['url']} is {pr['state'].lower()}, not merged yet.")
    commit = pr["mergeCommit"]["oid"]

    run("git", "fetch", "origin", BASE_BRANCH)
    if header_version(run("git", "show", f"{commit}:{VERSION_HEADER}")) != version:
        sys.exit(f"GA_VERSION_STR at {commit[:7]} is not {version}.")

    remote = remote_tag_commit(tag)
    local = local_tag_commit(tag)
    for where, existing in (("origin", remote), ("locally", local)):
        if existing and existing != commit:
            sys.exit(f"Tag {tag} already exists {where} at {existing[:7]}, not {commit[:7]}.")
    if not remote:
        if not local:
            run("git", "tag", "-a", tag, "-m", tag, commit)
        run("git", "push", "origin", f"refs/tags/{tag}")
    print(f"Tagged {commit[:7]} as {tag}")

    # The workflow builds the tip of main, so only trigger it while that is the tagged commit
    if run("git", "rev-parse", f"origin/{BASE_BRANCH}") != commit:
        sys.exit(f"origin/{BASE_BRANCH} has moved past {tag}, so the release build would not match the tag. "
                 f"Workflow not triggered.")
    trigger_workflow(tag)


def main():
    parser = argparse.ArgumentParser(description="Prepare and publish a C++ SDK release")
    commands = parser.add_subparsers(dest="command", required=True)

    prepare_parser = commands.add_parser("prepare", help="Bump the version, update CHANGELOG.md and open a release PR")
    prepare_parser.add_argument("version", help="Release version, e.g. 5.4.3")
    for category in CHANGELOG_CATEGORIES:
        prepare_parser.add_argument(f"--{category.lower()}", action="append", default=[], metavar="TEXT",
                                    help=f"Changelog entry under '### {category}' (repeatable)")
    prepare_parser.set_defaults(func=prepare)

    publish_parser = commands.add_parser("publish", help="Tag the merged release PR and trigger the Create Release workflow")
    publish_parser.add_argument("version", help="Release version, e.g. 5.4.3")
    publish_parser.set_defaults(func=publish)

    args = parser.parse_args()
    if not check_gh_cli_installed():
        sys.exit("GitHub CLI (gh) is not installed. Please install it and try again.")
    args.func(args)


if __name__ == "__main__":
    main()

# Publish Q3PSVR2 from Ubuntu (private GitHub repository)

Target: <https://github.com/tvk-source/Q3PSVR2>. The GitHub integration is authenticated as `tvk-source`, but **could not access the private repository** as of 2026-10-23. The initial push should come from the machine holding the complete source tree.

> **Important:** The ZIP containing these documentation files is **not the complete PS5 source repository**. Install it over the existing `~/Downloads/PS5_Quake3e` source tree and publish that tree only after review.

## 1. Prerequisites and authentication

```bash
sudo apt update
sudo apt install git gh python3 unzip

gh auth login --hostname github.com --git-protocol https --web
gh auth setup-git
gh auth status
```

GitHub CLI opens a browser/device-flow login. **Do not paste access tokens into this terminal, Git remote URLs, commits, or ChatGPT.**

If the folder is not already a Git repository:

```bash
cd ~/Downloads/PS5_Quake3e
if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  git init -b main
fi
```

Set the local Git commit identity. This uses GitHub's account ID and noreply-address convention to keep your personal email out of commits:

```bash
cd ~/Downloads/PS5_Quake3e
git config user.name "tvk-source"
GITHUB_ID="$(gh api user --jq .id)"
GITHUB_LOGIN="$(gh api user --jq .login)"
git config user.email "${GITHUB_ID}+${GITHUB_LOGIN}@users.noreply.github.com"
```

Do **not** delete or replace existing `.git/` metadata. If your checkout is already based on Quake3e or another Git repository, keep its history and remotes.

## 2. Prepare corresponding source and required attributions

- Include source for all Q3PSVR2-native modifications and any redistributed engine/VM changes, and the build scripts necessary to reproduce the build.
- If the gameplay QVM is only present as an experimental PK3, first export the actual modified QVM C-source patches into `patches/qvm-current/`, as described by the handoff's `docs/QVM_SOURCE.md`. **Do not assume a compiled PK3 alone satisfies GPL corresponding-source requirements.**
- Include all upstream GPL license and original copyright notices. Review `THIRD_PARTY_NOTICES.md` against the actual dependencies and licenses in the tree.
- Do **not** commit original Quake III commercial PAKs, firmware or decrypted `.sprx` files, optional third-party texture PK3s, private console debug files, credentials or build products. The `.gitignore` excludes typical outputs, but `git add -f` or previously tracked content can bypass it.
- Exclude `zzzzzz-q3psvr2-ui-menu-TEST.pk3`; it failed real headset testing. Keep your original TV/VR source tree, not just the documentation from this bundle.

## 3. Stage, audit, commit and push

```bash
cd ~/Downloads/PS5_Quake3e

git status --short
git add -A
python3 tools/check_git_content.py
git diff --cached --stat
# If the report is clean and the staged paths are intentional:
git commit -m "Initial Q3PSVR2 source and documentation"

# Adds a separate remote instead of replacing any upstream origin.
git remote add q3psvr2 https://github.com/tvk-source/Q3PSVR2.git
# If already added: git remote set-url q3psvr2 https://github.com/tvk-source/Q3PSVR2.git

git push -u q3psvr2 HEAD:main
```

**If the audit FAILS, do not commit:** remove offending staged entries with `git restore --staged -- <path>` (or `git rm --cached -- <path>` for tracked files) and run it again. Use `git diff --cached --name-only` to inspect every staged file before pushing.

If the GitHub repo was initialized with its own README or initial commit, a non-fast-forward rejection is possible. Do **not** use `--force`. Fetch its history first and reconcile the remote's files before retrying:

```bash
git fetch q3psvr2
git log --oneline --graph --decorate --all -12
```

If your `PS5_Quake3e` checkout is itself an existing fork/clone with a very large inherited history or nested submodules, audit `git remote -v`, `.gitmodules` and included licenses before pushing. A private repo should still avoid secrets and copyrighted assets.

## 4. Verify

```bash
gh repo view tvk-source/Q3PSVR2 --web
git status --short
git remote -v
```

Keep the repository **private** unless you explicitly decide to publish it. A GitHub source upload does not publish a playable PS5 release or confirm that every component is redistributable.

## Connector access alternative

In GitHub's installed app settings, grant ChatGPT's GitHub app access to `tvk-source/Q3PSVR2` (for a selected-repositories installation), or reconnect the app with appropriate repo access. Its authentication as `tvk-source` does not automatically imply it can see every private repository. A full initial local push is still preferable because the tested code resides on Ubuntu.

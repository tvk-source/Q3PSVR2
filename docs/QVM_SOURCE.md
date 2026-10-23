# Reproducible cgame QVM source

The shipping `vm/cgame.qvm` is built from the GPL Quake III Arena cgame, **not** embedded in the native PS5 executable. Reticle, weapon stereo, gun scaling, PCVR-inspired offsets and compact HUD currently live in the developer's isolated Ubuntu QVM source tree; it is important to commit source changes as well as a release PK3.

Export them as a patch against the same untouched GPL upstream checkout from which the sandbox was originally copied:

```sh
cd ~/Downloads/PS5_Quake3e
python3 tools/export_qvm_patches.py \
  --work "$PWD/ps5vr_vm_aim_test" \
  --upstream "$HOME/Downloads/Quake-III-Arena-vm" \
  --out "$PWD/patches/qvm-current"
```

The tool writes `patches/qvm-current/q3psvr2-cgame.patch` and SHA256 references for the two changed source files. **If the source checkout was reset, updated or diverged, stop and reconcile rather than forcing the patch.** To verify against an original checkout:

```sh
cd ~/Downloads/Quake-III-Arena-vm
git apply --check ~/Downloads/PS5_Quake3e/patches/qvm-current/q3psvr2-cgame.patch
```

On a clean worktree, `git apply` can then recreate the changes; use your existing `q3lcc/q3asm` build method to produce `vm/cgame.qvm` and its PK3. The release packaging script requires a prebuilt, validated PK3 and does not run the compiler for you. Track `patches/` in Git; never include decrypted PS5 firmware libraries or commercial `pak*.pk3`.

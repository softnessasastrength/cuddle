# WSL

`/mnt/c` is slow and copies files as mode 777. Build in `/tmp` or `$HOME`.

```
./build.sh
```

`build.sh` does not call apt/dnf/pacman. Install meson, ninja, and a C
compiler yourself. A user-local toolchain at
`~/.local/cuddle-tools/bin` is used when present.

PowerShell eats `$` inside `wsl -e bash -lc "..."`. Prefer a `.sh` file.

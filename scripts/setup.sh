#!/usr/bin/env bash
# switch-lm development environment setup.
#
#   scripts/setup.sh           install everything that is missing (asks for sudo)
#   scripts/setup.sh --check   only report what is installed / missing, change nothing
#
# Safe to re-run: every step skips work that is already done.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MANIFEST="$ROOT/dkp-packages.txt"
DEVKITPRO_DEFAULT="/opt/devkitpro"
CHECK_ONLY=0
[[ "${1:-}" == "--check" ]] && CHECK_ONLY=1

# ---------- output helpers ----------
if [[ -t 1 ]]; then B=$'\e[1m'; G=$'\e[32m'; Y=$'\e[33m'; R=$'\e[31m'; N=$'\e[0m'; else B= G= Y= R= N=; fi
step() { printf '\n%s==> %s%s\n' "$B" "$*" "$N"; }
ok()   { printf '  %s✓%s %s\n' "$G" "$N" "$*"; }
warn() { printf '  %s!%s %s\n' "$Y" "$N" "$*"; }
fail() { printf '  %s✗%s %s\n' "$R" "$N" "$*"; }
die()  { fail "$*"; exit 1; }
have() { command -v "$1" >/dev/null 2>&1; }

MISSING=0

# ---------- detect platform ----------
OS="$(uname -s)"
case "$OS" in
    Darwin) PLATFORM=macos ;;
    Linux)
        if [[ -f /etc/debian_version ]]; then PLATFORM=debian
        elif [[ -f /etc/arch-release ]]; then PLATFORM=arch
        else PLATFORM=linux-other; fi ;;
    MINGW*|MSYS*|CYGWIN*) PLATFORM=windows ;;
    *) PLATFORM=unknown ;;
esac
step "Platform: $PLATFORM ($(uname -m))"

# ---------- 1. devkitPro pacman ----------
step "devkitPro package manager (dkp-pacman)"

find_dkp_pacman() {
    if have dkp-pacman; then echo dkp-pacman
    elif [[ "$PLATFORM" == arch || "$PLATFORM" == windows ]] && have pacman; then echo pacman
    elif [[ -x "$DEVKITPRO_DEFAULT/pacman/bin/pacman" ]]; then echo "$DEVKITPRO_DEFAULT/pacman/bin/pacman"
    fi
}

PACMAN="$(find_dkp_pacman || true)"
if [[ -n "$PACMAN" ]]; then
    ok "found: $PACMAN"
elif (( CHECK_ONLY )); then
    fail "dkp-pacman not installed"; MISSING=1
else
    case "$PLATFORM" in
        macos)
            warn "installing devkitPro pacman (official .pkg from github.com/devkitPro/pacman)"
            tmp="$(mktemp -d)"
            curl -fL --retry 3 -o "$tmp/devkitpro-pacman-installer.pkg" \
                https://github.com/devkitPro/pacman/releases/latest/download/devkitpro-pacman-installer.pkg
            sudo installer -pkg "$tmp/devkitpro-pacman-installer.pkg" -target /
            rm -rf "$tmp" ;;
        debian)
            warn "installing devkitPro pacman (official script from apt.devkitpro.org)"
            tmp="$(mktemp -d)"
            curl -fL --retry 3 -o "$tmp/install-devkitpro-pacman" https://apt.devkitpro.org/install-devkitpro-pacman
            chmod +x "$tmp/install-devkitpro-pacman"
            sudo "$tmp/install-devkitpro-pacman"
            rm -rf "$tmp" ;;
        *)
            die "automatic devkitPro install isn't supported on '$PLATFORM'.
    Follow https://devkitpro.org/wiki/devkitPro_pacman, then re-run this script." ;;
    esac
    PACMAN="$(find_dkp_pacman || true)"
    [[ -n "$PACMAN" ]] || die "dkp-pacman still not found after install (open a new terminal and re-run?)"
    ok "installed: $PACMAN"
fi

# ---------- 2. devkitPro packages ----------
step "devkitPro packages ($(basename "$MANIFEST"))"

# Parallel arrays (macOS ships bash 3.2: no associative arrays).
PKGS=(); VERS=()
while read -r line || [[ -n "$line" ]]; do
    line="${line%%#*}"
    name=""; ver=""
    read -r name ver _ <<<"$line" || true
    [[ -z "$name" ]] && continue
    PKGS+=("$name"); VERS+=("$ver")
done < "$MANIFEST"

if [[ -n "$PACMAN" ]]; then
    if (( ! CHECK_ONLY )); then
        sudo "$PACMAN" -Sy --needed --noconfirm "${PKGS[@]}"
    fi
    for i in "${!PKGS[@]}"; do
        p="${PKGS[$i]}"; want="${VERS[$i]}"
        if "$PACMAN" -Qg "$p" >/dev/null 2>&1; then ok "$p (group)"; continue; fi
        inst="$("$PACMAN" -Q "$p" 2>/dev/null | awk '{print $2}')" || inst=""
        if [[ -z "$inst" ]]; then fail "$p not installed"; MISSING=1
        elif [[ -n "$want" && "$inst" != "$want" ]]; then
            warn "$p $inst (project last verified with $want)"
        else ok "$p $inst"; fi
    done
fi

# ---------- 3. DEVKITPRO environment variable ----------
step "DEVKITPRO environment variable"
if [[ -n "${DEVKITPRO:-}" ]]; then
    ok "DEVKITPRO=$DEVKITPRO"
elif [[ -d "$DEVKITPRO_DEFAULT" ]]; then
    warn "DEVKITPRO is not set in this shell. The Makefile falls back to $DEVKITPRO_DEFAULT,"
    warn "but other tools expect it. Add this to your ~/.zshrc or ~/.bashrc:"
    printf '      export DEVKITPRO=%s\n' "$DEVKITPRO_DEFAULT"
else
    fail "DEVKITPRO not set and $DEVKITPRO_DEFAULT does not exist"; MISSING=1
fi

# ---------- 4. host tools ----------
step "Host tools"
host_install() {
    case "$PLATFORM" in
        macos)  have brew || { warn "Homebrew not found (https://brew.sh); install '$1' manually"; return 1; }
                brew install "$1" ;;
        debian) sudo apt-get install -y "$1" ;;
        arch)   sudo pacman -S --needed --noconfirm "$1" ;;
        *)      warn "install '$1' manually"; return 1 ;;
    esac
}
for tool in git make cmake clang-format; do
    if have "$tool"; then ok "$tool"
    elif (( CHECK_ONLY )); then
        if [[ "$tool" == clang-format ]]; then warn "clang-format missing (pre-commit will skip format checks)"
        else fail "$tool missing"; MISSING=1; fi
    else
        warn "installing $tool"; host_install "$tool" || MISSING=1
    fi
done

# ---------- 4b. desktop build libraries (make mac) ----------
if [[ "$PLATFORM" == macos ]]; then
    step "Desktop build libraries (Homebrew, for make mac)"
    for f in sdl2 sdl2_ttf; do
        if have brew && brew list --versions "$f" >/dev/null 2>&1; then ok "$f"
        elif (( CHECK_ONLY )); then warn "$f missing (only needed for make mac)"
        else warn "installing $f"; host_install "$f" || warn "couldn't install $f; make mac won't work"; fi
    done
fi

# ---------- 5. git hooks ----------
step "Git hooks"
if git -C "$ROOT" rev-parse --git-dir >/dev/null 2>&1; then
    current="$(git -C "$ROOT" config --get core.hooksPath || true)"
    if [[ "$current" == ".githooks" ]]; then ok "core.hooksPath=.githooks"
    elif (( CHECK_ONLY )); then fail "pre-commit hook not enabled (run scripts/setup.sh)"; MISSING=1
    else git -C "$ROOT" config core.hooksPath .githooks; ok "enabled .githooks (pre-commit)"; fi
else
    warn "not a git checkout; skipping hooks"
fi

# ---------- summary ----------
step "Summary"
if (( MISSING )); then
    fail "some requirements are missing (see above)"
    (( CHECK_ONLY )) && printf '  run %sscripts/setup.sh%s to install them\n' "$B" "$N"
    exit 1
fi
ok "environment ready. Build with: make"

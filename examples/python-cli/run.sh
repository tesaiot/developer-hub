#!/usr/bin/env bash

# Copyright (c) 2025 TESAIoT Platform (TESA)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="$SCRIPT_DIR/.venv"
PYTHON_BIN="python3"

# Helper script to bootstrap a virtualenv and run the CLI in line with the README steps.

usage() {
  cat <<USAGE
Usage:
  $(basename "$0") bootstrap       # Create virtualenv and install dependencies
  $(basename "$0") run [args...]    # Run the app (virtualenv must exist)
USAGE
}

ensure_python() {
  # Ensure python3 exists in PATH so we fail early with a friendly tip.
  if ! command -v "$PYTHON_BIN" >/dev/null 2>&1; then
    echo "python3 is required" >&2
    exit 1
  fi
}

bootstrap() {
  # Mirrors the README "bootstrap" section: create venv, upgrade pip, install deps.
  ensure_python
  if [ ! -d "$VENV_DIR" ]; then
    "$PYTHON_BIN" -m venv "$VENV_DIR"
  fi
  source "$VENV_DIR/bin/activate"
  pip install --upgrade pip
  pip install -r "$SCRIPT_DIR/requirements.txt"
  echo "Dependency installation completed" >&2
}

run_app() {
  # Activate the virtualenv, then pass through any CLI parameters to Typer.
  ensure_python
  if [ ! -d "$VENV_DIR" ]; then
    echo "Virtualenv not found. Run '$0 bootstrap' first." >&2
    exit 1
  fi
  source "$VENV_DIR/bin/activate"
  PYTHONPATH="$SCRIPT_DIR" python "$SCRIPT_DIR/app.py" "$@"
}

if [ $# -lt 1 ]; then
  usage
  exit 1
fi

case "$1" in
  bootstrap)
    shift
    bootstrap "$@"
    ;;
  run)
    shift
    run_app "$@"
    ;;
  *)
    usage
    exit 1
    ;;
esac

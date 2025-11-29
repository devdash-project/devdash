# Development Scripts

This project includes several helper scripts in the `scripts/` directory to streamline common development tasks. These scripts are similar to npm scripts in Node.js projects.

## Available Scripts

### `./scripts/build`

Build the project with a specified CMake preset.

**Usage:**
```bash
./scripts/build [preset]
```

**Parameters:**
- `preset` (optional): CMake preset to use. Defaults to `dev` if not specified.

**Examples:**
```bash
# Build with dev preset (debug mode, sanitizers enabled)
./scripts/build

# Build with release preset (optimizations enabled)
./scripts/build release

# Build with CI preset
./scripts/build ci
```

**What it does:**
Runs `cmake --build build/$PRESET` to compile the project using the specified preset configuration.

---

### `./scripts/test`

Run the project's test suite using ctest.

**Usage:**
```bash
./scripts/test [preset]
```

**Parameters:**
- `preset` (optional): CMake preset to use. Defaults to `dev` if not specified.

**Examples:**
```bash
# Run tests for dev preset
./scripts/test

# Run tests for release preset
./scripts/test release
```

**What it does:**
Runs `ctest --test-dir build/$PRESET --output-on-failure` to execute all tests and show output for any failures.

---

### `./scripts/lint`

Run clang-tidy static analysis on the codebase.

**Usage:**
```bash
./scripts/lint [fix]
```

**Parameters:**
- `fix` (optional): When specified, automatically applies clang-tidy fixes to the code.

**Examples:**
```bash
# Check for linting issues (read-only)
./scripts/lint

# Automatically fix linting issues
./scripts/lint fix
```

**What it does:**
- Without `fix`: Runs `cmake --build build/dev --target clang-tidy` to check for code quality issues
- With `fix`: Runs `cmake --build build/dev --target clang-tidy-fix` to automatically fix issues when possible

**Note:** This requires the project to be configured and built at least once with the dev preset.

---

### `./scripts/format`

Format C++ code using clang-format.

**Usage:**
```bash
./scripts/format [check]
```

**Parameters:**
- `check` (optional): When specified, only checks formatting without modifying files.

**Examples:**
```bash
# Check formatting (useful in CI)
./scripts/format check

# Fix formatting issues
./scripts/format
```

**What it does:**
- Without `check`: Runs `cmake --build build/dev --target format-fix` to apply formatting changes
- With `check`: Runs `cmake --build build/dev --target format-check` to verify formatting without changes

---

### `./scripts/test-ci-local`

Test GitHub Actions workflows locally using `act`.

**Usage:**
```bash
sudo ./scripts/test-ci-local [job-name]
```

**Parameters:**
- `job-name` (optional): Specific CI job to run. If omitted, lists available jobs.

**Examples:**
```bash
# List all available CI jobs
sudo ./scripts/test-ci-local

# Run the build job locally
sudo ./scripts/test-ci-local build

# Run the lint job locally
sudo ./scripts/test-ci-local lint
```

**What it does:**
Uses [act](https://github.com/nektos/act) to run GitHub Actions workflows locally in Docker containers. This provides a tight feedback loop for CI testing without pushing to GitHub.

**Note:** Requires Docker and must be run with `sudo` to access the Docker daemon.

## Quick Reference

| Script | Purpose | Typical Usage |
|--------|---------|---------------|
| `build` | Compile project | `./scripts/build` |
| `test` | Run tests | `./scripts/test` |
| `lint` | Static analysis | `./scripts/lint` |
| `lint fix` | Auto-fix issues | `./scripts/lint fix` |
| `format` | Apply formatting | `./scripts/format` |
| `format check` | Check formatting | `./scripts/format check` |
| `test-ci-local` | Local CI testing | `sudo ./scripts/test-ci-local build` |

## Typical Workflow

### Before Committing

Run these scripts to ensure your code meets project standards:

```bash
# 1. Build the project
./scripts/build

# 2. Run tests
./scripts/test

# 3. Fix formatting
./scripts/format

# 4. Check for lint issues
./scripts/lint

# 5. (Optional) Auto-fix lint issues
./scripts/lint fix

# 6. Test CI locally before pushing
sudo ./scripts/test-ci-local build
```

### Quick Pre-Commit Check

```bash
# One-liner to run all checks
./scripts/build && ./scripts/test && ./scripts/format check && ./scripts/lint
```

## Integration with CI

These scripts use the same CMake targets as the CI pipeline, ensuring local results match CI results:

- `.github/workflows/ci.yml` uses the same targets
- `act` runs the actual GitHub Actions workflows
- No "works on my machine" surprises

## Requirements

All scripts require:
- CMake project configured at least once (`cmake --preset dev`)
- Build directory exists (`build/dev` for dev preset)

The `test-ci-local` script additionally requires:
- Docker installed and running
- `act` installed (included in dev container)
- Sufficient disk space for Docker images

## Troubleshooting

### "Target not found" errors

If you get errors about missing targets, reconfigure the project:

```bash
rm -rf build/dev
cmake --preset dev
./scripts/build
```

### clang-tidy/clang-format not found

These tools should be installed in the dev container. If running outside the container:

```bash
sudo apt-get install clang-tidy clang-format
```

### act requires sudo

The `test-ci-local` script needs Docker access. Either:
- Run with `sudo` (recommended)
- Add your user to the `docker` group (requires logout/login)

```bash
sudo usermod -aG docker $USER
# Log out and back in for changes to take effect
```

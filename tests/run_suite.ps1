# file: run_suite.ps1
#
# Builds test_exe and runs it with one gtest filter, from build/bin/<configuration>.
#
# This is the one definition of how a mutation plan runs the suite. Every plan under
# tests/ names this script in its test_command, and MutationPlanTest refuses a plan
# that does not. The suite runs from build/bin/<configuration> because that is the
# working directory ci-tests.yml, CTest and the debugger use; a test that opens a
# bare relative path passes from the repository root and fails in CI.
#
# Run it, from any directory:
#
#     powershell -NoProfile -ExecutionPolicy Bypass -File tests/run_suite.ps1 -Configuration Debug -Filter "--gtest_filter=WebSpinnerSanctuaryTest.*:WebSpinnerQuarryTest.*"
#
#     exit 0   every test the filter names passed
#     exit 1   the build failed, so test_exe never ran
#     exit N   test_exe's own exit code, non-zero when a test failed
#
# -Configuration is Debug or Release, and anything else is refused before the build.
# -Filter is passed to test_exe exactly as given, and is named rather than placed,
# because an argument starting with '-' is not a safe positional value.
#
# Each call costs one build of test_exe. With nothing changed that build only checks
# timestamps, which is seconds rather than minutes, and it is paid again per mutant.
# Build output is discarded; mutate.py reads only the exit code.

param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration,

    [Parameter(Mandatory = $true)]
    [string]$Filter
)

# The repository root is this script's parent folder, whatever directory it was called from.
Set-Location (Split-Path -Parent $PSScriptRoot)

cmake --build build --config $Configuration --target test_exe > $null
if ($LASTEXITCODE -ne 0)
{
    exit 1
}

Set-Location "build/bin/$Configuration"
./test_exe.exe $Filter
exit $LASTEXITCODE

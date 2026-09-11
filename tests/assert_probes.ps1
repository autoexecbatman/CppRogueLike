# file: assert_probes.ps1
#
# Falsifies every probe in tests/Core/AssertProbeTest.cpp.
#
# Each probe claims that one assertion fires when its invariant is broken. A
# green run of the probes cannot establish that: it shows the process died with
# the right text on stderr, and an assertion that was never there at all would
# also leave the suite green if nothing else in the tree happened to reach it.
# This deletes the targeted assertion, rebuilds, runs that probe alone and
# expects it to FAIL. A probe that still passes without its assertion is
# measuring something other than the assertion.
#
# The source is restored from the bytes read before the edit, and a recovery
# copy is written to disk first: a build that is killed never reaches the
# restore, and a repository left with a deleted assertion is worse than no
# measurement. A leftover recovery file is restored at startup.
#
# Run it, from the repository root:
#
#     powershell -NoProfile -ExecutionPolicy Bypass -File tests/assert_probes.ps1
#
#     baseline builds and all probes pass; each one is falsified below
#
#       WearingNothingAborts          FALSIFIED - the probe fails without its assertion
#       ...
#     6 of 6 probes falsified
#
# It rebuilds test_exe once per probe, so it takes minutes rather than seconds.
# Run it when a probe is added, when one of the targeted assertions is touched,
# and before trusting a green probe run for anything.

# -RecoverOnly restores an interrupted run's sources and stops, without building
# anything. It exists so the recovery path can be exercised in a second rather
# than only by killing a real run.
param([switch]$RecoverOnly)

$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root "build/bin/Debug/test_exe.exe"

# Each probe names the assertion it claims to exercise. The anchor is matched
# against whole lines, and a run stops if it does not match exactly one - a
# vacuous deletion would report the probe as falsified while changing nothing.
$probes = @(
    @{
        test   = "AssertProbeDeathTest.WearingNothingAborts"
        file   = "src/Actor/Creature.cpp"
        anchor = 'assert(item && "Creature::wear called with no item");'
    },
    @{
        test   = "AssertProbeDeathTest.WearingIntoASlotTheBodyLacksAborts"
        file   = "src/Actor/Creature.cpp"
        anchor = 'assert(has_slot(slot) && "Creature::wear called with a slot this body does not have");'
    },
    @{
        test   = "AssertProbeDeathTest.BuildingAMonsterWithNoBodyPlanRegistryAborts"
        file   = "src/Factories/MonsterCreator.cpp"
        anchor = 'assert(ctx.bodyPlanRegistry && "MonsterCreator::create_from_params called without a bodyPlanRegistry");'
    },
    @{
        test   = "AssertProbeDeathTest.BuildingAMonsterWithNoContentRegistryAborts"
        file   = "src/Factories/MonsterCreator.cpp"
        anchor = 'assert(ctx.contentRegistry && "MonsterCreator::create_from_params called without a contentRegistry");'
    },
    @{
        test   = "AssertProbeDeathTest.ConfiguringAShopkeeperWithNoContentRegistryAborts"
        file   = "src/Systems/Shopkeepers/ShopkeeperFactory.cpp"
        anchor = 'assert(ctx.contentRegistry && "configure_shopkeeper called without a contentRegistry");'
    },
    @{
        test   = "AssertProbeDeathTest.KillingACreatureWithNoPlayerInContextAborts"
        file   = "src/Actor/Creature.cpp"
        anchor = 'assert(ctx.player() != nullptr && "Creature::die requires a live player in context");'
    }
)

function Get-RecoveryPath($sourcePath)
{
    return "$sourcePath.assert_probe_recovery"
}

# A previous run that was killed mid-build leaves the source without its
# assertion. Put it back before measuring anything.
foreach ($probe in ($probes | ForEach-Object { $_.file } | Sort-Object -Unique))
{
    $sourcePath = Join-Path $root $probe
    $recovery = Get-RecoveryPath $sourcePath
    if (Test-Path $recovery)
    {
        [IO.File]::WriteAllBytes($sourcePath, [IO.File]::ReadAllBytes($recovery))
        Remove-Item $recovery -Force
        Write-Host "recovered $probe from an interrupted run"
    }
}

if ($RecoverOnly)
{
    exit 0
}

function Invoke-Build
{
    $output = & cmake --build (Join-Path $root "build") --config Debug --target test_exe 2>&1 | Out-String
    return @{ ok = ($LASTEXITCODE -eq 0); output = $output }
}

function Invoke-Probe($testName)
{
    Push-Location (Split-Path $exe -Parent)
    $null = & $exe "--gtest_filter=$testName" 2>&1 | Out-String
    $code = $LASTEXITCODE
    Pop-Location
    return $code
}

# The unmutated tree, built and run exactly as every falsification will be.
# Without this a broken build reads as every probe falsified, and a harness that
# measures nothing is indistinguishable from one that measured everything.
Write-Host "baseline: building and running all probes"
$build = Invoke-Build
if (-not $build.ok)
{
    Write-Host "BASELINE DOES NOT BUILD - every probe would read as falsified. Stopping."
    Write-Host $build.output
    exit 1
}
if ((Invoke-Probe "AssertProbeDeathTest.*") -ne 0)
{
    Write-Host "BASELINE PROBES FAIL - a falsification would mean nothing. Stopping."
    exit 1
}
Write-Host "baseline builds and all probes pass; each one is falsified below`n"

$falsified = 0
foreach ($probe in $probes)
{
    $sourcePath = Join-Path $root $probe.file
    $originalBytes = [IO.File]::ReadAllBytes($sourcePath)
    $text = [Text.Encoding]::UTF8.GetString($originalBytes)

    # Whole lines, so a deletion cannot leave half an expression behind.
    $lines = $text -split "`n"
    $matching = @($lines | Where-Object { $_.Trim() -eq $probe.anchor })
    if ($matching.Count -ne 1)
    {
        Write-Host ("  {0,-52} ANCHOR MATCHED {1} LINES - deletion would be vacuous" -f $probe.test.Replace("AssertProbeDeathTest.", ""), $matching.Count)
        continue
    }

    $kept = $lines | Where-Object { $_.Trim() -ne $probe.anchor }
    $recovery = Get-RecoveryPath $sourcePath
    [IO.File]::WriteAllBytes($recovery, $originalBytes)
    [IO.File]::WriteAllBytes($sourcePath, [Text.Encoding]::UTF8.GetBytes(($kept -join "`n")))

    $build = Invoke-Build
    if ($build.ok)
    {
        $code = Invoke-Probe $probe.test
    }

    # Restored before the verdict is printed, so an unreadable verdict still
    # leaves the tree intact.
    [IO.File]::WriteAllBytes($sourcePath, $originalBytes)
    if ([IO.File]::ReadAllBytes($sourcePath).Length -ne $originalBytes.Length)
    {
        Write-Host "RESTORE FAILED for $($probe.file) - recovery copy is at $recovery. Stopping."
        exit 1
    }
    Remove-Item $recovery -Force

    $label = $probe.test.Replace("AssertProbeDeathTest.", "")
    if (-not $build.ok)
    {
        Write-Host ("  {0,-52} FALSIFIED by the compiler" -f $label)
        $falsified++
    }
    elseif ($code -ne 0)
    {
        Write-Host ("  {0,-52} FALSIFIED - the probe fails without its assertion" -f $label)
        $falsified++
    }
    else
    {
        Write-Host ("  {0,-52} STILL PASSES without its assertion - it proves nothing" -f $label)
    }
}

# The tree is rebuilt whole, so the next ordinary test run is not against a
# binary built from a deleted assertion.
$null = Invoke-Build
Write-Host ("`n{0} of {1} probes falsified" -f $falsified, $probes.Count)

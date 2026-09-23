param([switch]$Test)
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$buildTools = & $locator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.14.44.17.14.x86.x64 -property installationPath
if(-not $buildTools) { throw 'Install v143 x64/x86 tools, MFC/ATL and their Spectre libraries in Build Tools.' }
$msbuild = Join-Path $buildTools 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild (Join-Path $projectRoot 'openmpt-src\build\vs2022win10\OpenMPT.vcxproj') /m /nr:false /t:Build /p:Configuration=Debug /p:Platform=x64 /p:WindowsTargetPlatformVersion=10.0.26100.0 /v:minimal
if($LASTEXITCODE -ne 0) { throw "Native build failed: $LASTEXITCODE" }
if($Test) {
    $aiEnvVars = 'OPENMPT_AI_TEST_FIXTURE', 'OPENMPT_PIANOROLL_REGRESSION_FIXTURE', 'OPENMPT_AI_TEST_REPORT', 'OPENMPT_AI_DEMO_REPORT'
    $savedEnv = @{}
    foreach($name in $aiEnvVars) { $savedEnv[$name] = [Environment]::GetEnvironmentVariable($name) }
    $testReport = Join-Path $projectRoot '.scratch\ai-native-test.txt'
    $demoReport = Join-Path $projectRoot '.scratch\ai-pattern-demo.json'
    try {
        $env:OPENMPT_AI_TEST_FIXTURE = Join-Path $projectRoot 'test-fixtures\ai-collab-fixture.mptm'
        $env:OPENMPT_AI_TEST_REPORT = $testReport
        $env:OPENMPT_AI_DEMO_REPORT = $demoReport
        if(Test-Path -LiteralPath $testReport) { Remove-Item -LiteralPath $testReport }
        if(Test-Path -LiteralPath $demoReport) { Remove-Item -LiteralPath $demoReport }
        Start-Process -FilePath (Join-Path $projectRoot 'openmpt-src\bin\debug\vs2022-win10-static\amd64\OpenMPT.exe') -ArgumentList '/noSysCheck','/noTests','/noPlugins','/noDls' -WindowStyle Hidden -Wait
        if(-not (Test-Path -LiteralPath $testReport)) { throw 'Native test runner produced no report.' }
        $result = Get-Content -LiteralPath $testReport -Raw
        Write-Output $result
        if($result.Trim() -ne 'PASS') { throw 'Native capability tests failed.' }
        if(-not (Test-Path -LiteralPath $demoReport)) { throw 'Capability tests passed but produced no demo report.' }
        Get-Content -LiteralPath $demoReport -Raw
        $realProjectFixture = Join-Path $projectRoot 'test-fixtures\th04_15_betafinalmix.mptm'
        if(Test-Path -LiteralPath $realProjectFixture) {
            $env:OPENMPT_AI_TEST_FIXTURE = $null
            $env:OPENMPT_PIANOROLL_REGRESSION_FIXTURE = $realProjectFixture
            if(Test-Path -LiteralPath $testReport) { Remove-Item -LiteralPath $testReport }
            Start-Process -FilePath (Join-Path $projectRoot 'openmpt-src\bin\debug\vs2022-win10-static\amd64\OpenMPT.exe') -ArgumentList '/noSysCheck','/noTests','/noPlugins','/noDls' -WindowStyle Hidden -Wait
            if(-not (Test-Path -LiteralPath $testReport)) { throw 'Real-project Piano Roll regression produced no report.' }
            $realProjectResult = Get-Content -LiteralPath $testReport -Raw
            Write-Output "Real-project Piano Roll regression: $($realProjectResult.Trim())"
            if($realProjectResult.Trim() -ne 'PASS') { throw 'Real-project Piano Roll regression failed.' }
        }
    } finally {
        foreach($name in $aiEnvVars) { [Environment]::SetEnvironmentVariable($name, $savedEnv[$name]) }
    }
}

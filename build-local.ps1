param([switch]$Test)
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
$locator = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$buildTools = & $locator -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.14.44.17.14.x86.x64 -property installationPath
if(-not $buildTools) { throw 'Install v143 x64/x86 tools, MFC/ATL and their Spectre libraries in Build Tools.' }
$msbuild = Join-Path $buildTools 'MSBuild\Current\Bin\MSBuild.exe'
& $msbuild (Join-Path $projectRoot 'openmpt-original_ref\build\vs2022win10\OpenMPT.vcxproj') /m /nr:false /t:Build /p:Configuration=Debug /p:Platform=x64 /p:WindowsTargetPlatformVersion=10.0.26100.0 /v:minimal
if($LASTEXITCODE -ne 0) { throw "Native build failed: $LASTEXITCODE" }
if($Test) {
    $savedFixture = $env:OPENMPT_AI_TEST_FIXTURE
    $savedReport = $env:OPENMPT_AI_TEST_REPORT
    $testReport = Join-Path $projectRoot '.scratch\ai-native-test.txt'
    try {
        $env:OPENMPT_AI_TEST_FIXTURE = Join-Path $projectRoot 'test-fixtures\ai-collab-fixture.mptm'
        $env:OPENMPT_AI_TEST_REPORT = $testReport
        if(Test-Path -LiteralPath $testReport) { Remove-Item -LiteralPath $testReport }
        Start-Process -FilePath (Join-Path $projectRoot 'openmpt-original_ref\bin\debug\vs2022-win10-static\amd64\OpenMPT.exe') -ArgumentList '/noSysCheck','/noTests','/noPlugins','/noDls' -WindowStyle Hidden -Wait
        $result = Get-Content -LiteralPath $testReport -Raw
        Write-Output $result
        if($result.Trim() -ne 'PASS') { throw 'Native capability tests failed.' }
    } finally {
        $env:OPENMPT_AI_TEST_FIXTURE = $savedFixture
        $env:OPENMPT_AI_TEST_REPORT = $savedReport
    }
}

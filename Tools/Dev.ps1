if (-not $global:MarchDevEnvActivated) {
    $env:Path = "$PSScriptRoot\DevEnv;$env:Path"

    function global:MarchDevOldPrompt { "" }
    Copy-Item -Path function:prompt -Destination function:MarchDevOldPrompt

    function global:prompt {
        Write-Host -NoNewline -ForegroundColor Magenta "(MarchDev) "
        MarchDevOldPrompt
    }

    $global:MarchDevEnvActivated = $true
}
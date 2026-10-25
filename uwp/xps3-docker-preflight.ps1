param(
    [switch]$RebuildImage
)

$ErrorActionPreference = 'Stop'
$image = 'xps3-uwp-builder:2026-10-25'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$localDocker = Join-Path $env:LOCALAPPDATA 'Programs\DockerDesktop\resources\bin\docker.exe'
$dockerCommand = Get-Command docker -ErrorAction SilentlyContinue
$docker = if ($dockerCommand) { $dockerCommand.Source } elseif (Test-Path -LiteralPath $localDocker) { $localDocker } else { throw 'Docker CLI not found' }

& $docker version --format 'Docker client={{.Client.Version}} server={{.Server.Version}}'
if ($LASTEXITCODE -ne 0) { throw 'Docker Desktop Linux engine is not ready' }

& $docker image inspect $image *> $null
$imageExists = $LASTEXITCODE -eq 0
if ($RebuildImage -or -not $imageExists) {
    & $docker build --progress=plain `
        -f (Join-Path $root 'uwp\toolchain\Dockerfile.xps3-uwp') `
        -t $image `
        (Join-Path $root 'uwp\toolchain')
    if ($LASTEXITCODE -ne 0) { throw 'XPS3 builder image failed' }
}

$arguments = @(
    'run', '--rm', '--memory=1700m', '--cpus=4',
    '-e', 'XPS3_TOOLCHAIN_CACHE=/xps3-cache/preflight',
    '--mount', "type=bind,source=$root,target=/src,readonly",
    '--mount', 'type=volume,source=xps3-toolchain-cache,target=/xps3-cache',
    '--mount', 'type=volume,source=xps3-zig-cache,target=/root/.cache/zig',
    $image,
    'sh', '/src/uwp/xps3-docker-preflight.sh'
)

& $docker @arguments
if ($LASTEXITCODE -ne 0) { throw 'XPS3 UWP toolchain preflight failed' }

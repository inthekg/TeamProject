# =====================================================================
#  normalize-encoding.ps1  (1회 실행용)
#  소스 인코딩 일괄 정규화
#    - 일반 소스(cpp/h/inl 등) -> UTF-8 (BOM 포함)
#    - 셰이더(hlsl/hlsli)      -> UTF-8 (BOM 없음)  ※ BOM 붙으면 셰이더 컴파일 문제
#    - 판별: UTF-8로 읽히면 UTF-8/ASCII, 아니면 CP949 (무손실)
#            둘 다 아니면(혼합/손상 의심) 건드리지 않고 경고만 출력
#    - 내용/줄끝 보존, 인코딩만 변경
#
#  [실행법] git 저장소 안 아무 위치에서:
#     powershell -ExecutionPolicy Bypass -File scripts\normalize-encoding.ps1
# =====================================================================

$ErrorActionPreference = 'Stop'

# --- git 저장소 확인 + 루트로 이동 (하위 폴더에서 실행해도 전체 파일 대상) ---
$root = cmd /c "git rev-parse --show-toplevel 2>nul"
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($root)) {
    Write-Host "[오류] git 저장소 안에서 실행하세요. (현재 위치: $(Get-Location))" -ForegroundColor Red
    exit 1
}
Set-Location $root

# --- 대상 파일 (git이 추적하는 소스만) ---
$files = git -c core.quotepath=false ls-files |
         Where-Object { $_ -match '\.(cpp|cc|cxx|c|h|hpp|hh|hxx|inl|hlsl|hlsli)$' }

# --- 인코딩 객체 ---
$utf8Strict  = New-Object System.Text.UTF8Encoding($false, $true)   # 판별용: 잘못된 바이트면 throw
$cp949Strict = [System.Text.Encoding]::GetEncoding(949,
                 [System.Text.EncoderFallback]::ExceptionFallback,
                 [System.Text.DecoderFallback]::ExceptionFallback)  # 판별용: 잘못된 바이트면 throw
$utf8Bom     = New-Object System.Text.UTF8Encoding($true)           # 일반 소스 목표
$utf8NoBom   = New-Object System.Text.UTF8Encoding($false)          # 셰이더 목표

$conv = 0; $shaderConv = 0; $keep = 0
$bad  = New-Object System.Collections.Generic.List[string]

foreach ($f in $files) {
    $path  = (Resolve-Path -LiteralPath $f).Path
    $bytes = [System.IO.File]::ReadAllBytes($path)

    $hasBom   = ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
    $isShader = ($f -match '\.(hlsl|hlsli)$')

    if ($isShader) {
        # ---------- 셰이더: UTF-8 (BOM 없음) ----------
        if ($hasBom) {
            # BOM 제거 후 본문 판별 -> 무BOM으로 재기록
            $body = New-Object byte[] ($bytes.Length - 3)
            if ($body.Length -gt 0) { [Array]::Copy($bytes, 3, $body, 0, $body.Length) }
            $text = $null
            try { $text = $utf8Strict.GetString($body) }
            catch { try { $text = $cp949Strict.GetString($body) } catch { } }
            if ($null -eq $text) { $bad.Add($f); continue }
            [System.IO.File]::WriteAllText($path, $text, $utf8NoBom)
            $shaderConv++
        }
        else {
            $isUtf8 = $true
            try { [void]$utf8Strict.GetString($bytes) } catch { $isUtf8 = $false }
            if ($isUtf8) { $keep++; continue }          # 이미 UTF-8/ASCII 무BOM = 목표 상태
            $text = $null
            try { $text = $cp949Strict.GetString($bytes) } catch { }
            if ($null -eq $text) { $bad.Add($f); continue }
            [System.IO.File]::WriteAllText($path, $text, $utf8NoBom)
            $shaderConv++
        }
        continue
    }

    # ---------- 일반 소스: UTF-8 (BOM 포함) ----------
    if ($hasBom) { $keep++; continue }                  # 이미 UTF-8 BOM = 목표 상태
    $text = $null
    try { $text = $utf8Strict.GetString($bytes) }
    catch { try { $text = $cp949Strict.GetString($bytes) } catch { } }
    if ($null -eq $text) { $bad.Add($f); continue }
    [System.IO.File]::WriteAllText($path, $text, $utf8Bom)
    $conv++
}

Write-Host ""
Write-Host "[완료] 일반 소스 변환(-> UTF-8 BOM) = $conv"
Write-Host "       셰이더 변환(-> UTF-8 무BOM)  = $shaderConv"
Write-Host "       이미 목표 상태(유지)          = $keep"
Write-Host "       전체 대상                     = $($files.Count)"

if ($bad.Count -gt 0) {
    Write-Host ""
    Write-Host "[경고] 인코딩 판별 실패(혼합/손상 의심) - 건드리지 않았음. 수동 확인 필요:" -ForegroundColor Yellow
    $bad | ForEach-Object { Write-Host "  $_" -ForegroundColor Yellow }
    exit 2
}

Write-Host ""
Write-Host "다음 단계: git add --renormalize . / git add -A / git commit / git push"

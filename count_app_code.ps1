Param(
    [string]$AppPath = "$PSScriptRoot\App",
    [string]$OutFile = "$PSScriptRoot\App\app_code_stats.txt"
)

if (-not (Test-Path $AppPath)) {
    Write-Error "App directory not found: $AppPath"
    exit 1
}

# 收集文件（只统计源码/头文件）
$files = Get-ChildItem -Path $AppPath -Recurse -File -Include *.c,*.cpp,*.h,*.hpp
if ($files.Count -eq 0) {
    Write-Warning "No source files found (*.c/*.cpp/*.h/*.hpp)."
}

function Get-CodeLinesInfo {
    param([string]$FullPath)
    $raw = Get-Content -Raw -LiteralPath $FullPath
    $lines = $raw -split "`n"
    $rawCount = $lines.Count
    $nonEmptyCount = ($lines | Where-Object { $_.Trim() -ne "" }).Count

    # 简易代码行统计：排除纯注释行与空行（不展开多行块注释的精细解析）
    $codeLines = 0
    $inBlock = $false
    foreach ($l in $lines) {
        $t = $l.Trim()
        if ($t -eq "") { continue }
        # 处理块注释开始/结束
        if ($t -match '/\*' -and $t -match '\*/') {
            # 单行块注释整行忽略
            continue
        } elseif ($t -match '/\*') { $inBlock = $true; continue }
        elseif ($t -match '\*/') { $inBlock = $false; continue }
        if ($inBlock) { continue }
        if ($t -like '//*') { continue }
        $codeLines++
    }

    [PSCustomObject]@{
        File          = ($FullPath.Substring($AppPath.Length) -replace '^[\\/]')
        RawLines      = $rawCount
        NonEmptyLines = $nonEmptyCount
        CodeLines     = $codeLines
    }
}

$report = foreach ($f in $files) { Get-CodeLinesInfo -FullPath $f.FullName }

$totalRaw      = ($report | Measure-Object RawLines -Sum).Sum
$totalNonEmpty = ($report | Measure-Object NonEmptyLines -Sum).Sum
$totalCode     = ($report | Measure-Object CodeLines -Sum).Sum

# 按顶层子目录聚合
$group = $report | ForEach-Object {
    $top = ($_.File -split '[\\/]')[0]
    if ($top -eq "") { $top = "." }
    [PSCustomObject]@{ Group=$top; RawLines=$_.RawLines; NonEmptyLines=$_.NonEmptyLines; CodeLines=$_.CodeLines }
} | Group-Object Group | ForEach-Object {
    [PSCustomObject]@{
        Group         = $_.Name
        RawLines      = ($_.Group | Measure-Object RawLines -Sum).Sum
        NonEmptyLines = ($_.Group | Measure-Object NonEmptyLines -Sum).Sum
        CodeLines     = ($_.Group | Measure-Object CodeLines -Sum).Sum
    }
}

# 构建输出内容
$sb = New-Object System.Text.StringBuilder
$null = $sb.AppendLine("ChipTanks App Code Stats")
$null = $sb.AppendLine("Time: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
$null = $sb.AppendLine("Directory: $AppPath")
$null = $sb.AppendLine("")
$null = $sb.AppendLine("Per-file:")
foreach ($r in $report | Sort-Object File) {
    $null = $sb.AppendLine(('{0,-55} Raw={1,6} NonEmpty={2,6} Code={3,6}' -f $r.File, $r.RawLines, $r.NonEmptyLines, $r.CodeLines))
}
$null = $sb.AppendLine("")
$null = $sb.AppendLine("Top-level groups:")
foreach ($g in $group | Sort-Object Group) {
    $null = $sb.AppendLine(('{0,-15} Raw={1,6} NonEmpty={2,6} Code={3,6}' -f $g.Group, $g.RawLines, $g.NonEmptyLines, $g.CodeLines))
}
$null = $sb.AppendLine("")
$null = $sb.AppendLine("Totals: RawLines=$totalRaw  NonEmptyLines=$totalNonEmpty  CodeLines=$totalCode")
$null = $sb.AppendLine("")
$null = $sb.AppendLine("Note: CodeLines is an estimate (ignores block comments and // lines).")
$null = $sb.AppendLine("Improvements: parse multi-line comments, exclude generated files, export CSV, incremental analysis.")

$dir = Split-Path $OutFile -Parent
if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
$sb.ToString() | Out-File -LiteralPath $OutFile -Encoding UTF8 -Force

Write-Host 'Done -> ' $OutFile -ForegroundColor Green

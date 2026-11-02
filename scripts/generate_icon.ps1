# ==============================================================================
# Dash Competitivo Audio - Generador Vectorial de Icono Táctico (.ico)
# Genera icon.ico multirresolución (256x256, 48x48, 32x32, 16x16) con GDI+ (.NET)
# Cero dependencias externas. 100% nativo de Windows.
# ==============================================================================

Add-Type -AssemblyName System.Drawing

function Draw-TacticalSkull([int]$size) {
    $bmp = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)

    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)

    $scale = [float]$size

    # 1. Placa/Emblema oscuro de fondo
    $bgRect = New-Object System.Drawing.RectangleF(
        ($scale * 0.04), ($scale * 0.04),
        ($scale * 0.92), ($scale * 0.92)
    )
    $bgBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 14, 17, 23)) # #0E1117
    $cornerRad = $scale * 0.22

    $bgPath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $bgPath.AddArc($bgRect.X, $bgRect.Y, $cornerRad, $cornerRad, 180, 90)
    $bgPath.AddArc(($bgRect.Right - $cornerRad), $bgRect.Y, $cornerRad, $cornerRad, 270, 90)
    $bgPath.AddArc(($bgRect.Right - $cornerRad), ($bgRect.Bottom - $cornerRad), $cornerRad, $cornerRad, 0, 90)
    $bgPath.AddArc($bgRect.X, ($bgRect.Bottom - $cornerRad), $cornerRad, $cornerRad, 90, 90)
    $bgPath.CloseFigure()

    $g.FillPath($bgBrush, $bgPath)

    # Borde de neón cian (#06B6D4)
    $borderPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 6, 182, 212), ($scale * 0.035))
    $g.DrawPath($borderPen, $bgPath)

    # 2. Silueta de la calavera
    $skullPath = New-Object System.Drawing.Drawing2D.GraphicsPath
    # Bóveda craneal
    $skullPath.StartFigure()
    $skullPath.AddBezier(
        [System.Drawing.PointF]::new($scale * 0.24, $scale * 0.46),
        [System.Drawing.PointF]::new($scale * 0.22, $scale * 0.16),
        [System.Drawing.PointF]::new($scale * 0.78, $scale * 0.16),
        [System.Drawing.PointF]::new($scale * 0.76, $scale * 0.46)
    )
    # Pómulo derecho y hendidura
    $skullPath.AddLine([System.Drawing.PointF]::new($scale * 0.72, $scale * 0.58), [System.Drawing.PointF]::new($scale * 0.66, $scale * 0.58))
    # Mandíbula y mentón
    $skullPath.AddLine([System.Drawing.PointF]::new($scale * 0.64, $scale * 0.80), [System.Drawing.PointF]::new($scale * 0.36, $scale * 0.80))
    # Mandíbula izquierda y pómulo
    $skullPath.AddLine([System.Drawing.PointF]::new($scale * 0.34, $scale * 0.58), [System.Drawing.PointF]::new($scale * 0.28, $scale * 0.58))
    $skullPath.CloseFigure()

    # Relleno de degradado cian táctico
    $gradBrush = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
        [System.Drawing.PointF]::new($scale * 0.5, $scale * 0.16),
        [System.Drawing.PointF]::new($scale * 0.5, $scale * 0.80),
        [System.Drawing.Color]::FromArgb(255, 34, 211, 238), # #22D3EE
        [System.Drawing.Color]::FromArgb(255, 6, 182, 212)   # #06B6D4
    )
    $g.FillPath($gradBrush, $skullPath)

    # Contorno sutil
    $skullPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(180, 255, 255, 255), ($scale * 0.02))
    $g.DrawPath($skullPen, $skullPath)

    # 3. Cuencas oculares angulares e inclinadas
    $darkBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 14, 17, 23))

    $leftEye = New-Object System.Drawing.Drawing2D.GraphicsPath
    $leftEye.AddPolygon([System.Drawing.PointF[]]@(
        [System.Drawing.PointF]::new($scale * 0.32, $scale * 0.40),
        [System.Drawing.PointF]::new($scale * 0.45, $scale * 0.45),
        [System.Drawing.PointF]::new($scale * 0.42, $scale * 0.56),
        [System.Drawing.PointF]::new($scale * 0.33, $scale * 0.50)
    ))
    $g.FillPath($darkBrush, $leftEye)

    $rightEye = New-Object System.Drawing.Drawing2D.GraphicsPath
    $rightEye.AddPolygon([System.Drawing.PointF[]]@(
        [System.Drawing.PointF]::new($scale * 0.68, $scale * 0.40),
        [System.Drawing.PointF]::new($scale * 0.55, $scale * 0.45),
        [System.Drawing.PointF]::new($scale * 0.58, $scale * 0.56),
        [System.Drawing.PointF]::new($scale * 0.67, $scale * 0.50)
    ))
    $g.FillPath($darkBrush, $rightEye)

    # 4. Cavidad nasal triangular
    $nose = New-Object System.Drawing.Drawing2D.GraphicsPath
    $nose.AddPolygon([System.Drawing.PointF[]]@(
        [System.Drawing.PointF]::new($scale * 0.50, $scale * 0.56),
        [System.Drawing.PointF]::new($scale * 0.46, $scale * 0.65),
        [System.Drawing.PointF]::new($scale * 0.54, $scale * 0.65)
    ))
    $g.FillPath($darkBrush, $nose)

    # 5. Ranuras de dientes en la mandíbula
    $toothPen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(255, 14, 17, 23), [Math]::Max(1.0, ($scale * 0.035)))
    $g.DrawLine($toothPen, ($scale * 0.42), ($scale * 0.70), ($scale * 0.42), ($scale * 0.80))
    $g.DrawLine($toothPen, ($scale * 0.50), ($scale * 0.70), ($scale * 0.50), ($scale * 0.80))
    $g.DrawLine($toothPen, ($scale * 0.58), ($scale * 0.70), ($scale * 0.58), ($scale * 0.80))

    $g.Dispose()
    return $bmp
}

function Save-MultiResIco([string]$outputPath) {
    $sizes = @(256, 48, 32, 16)
    $pngBuffers = @()

    foreach ($s in $sizes) {
        $bmp = Draw-TacticalSkull -size $s
        $ms = New-Object System.IO.MemoryStream
        $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
        $pngBuffers += ,($ms.ToArray())
        $ms.Dispose()
        $bmp.Dispose()
    }

    $fs = New-Object System.IO.FileStream($outputPath, [System.IO.FileMode]::Create, [System.IO.FileAccess]::Write)
    $bw = New-Object System.IO.BinaryWriter($fs)

    # ICONDIR
    $bw.Write([uint16]0) # Reserved
    $bw.Write([uint16]1) # Type (1 = ICO)
    $bw.Write([uint16]$sizes.Count) # Image count

    # Offset to image data = Header (6 bytes) + Directory Entries (16 * count)
    $offset = 6 + (16 * $sizes.Count)

    for ($i = 0; $i -lt $sizes.Count; $i++) {
        $w = if ($sizes[$i] -ge 256) { 0 } else { [byte]$sizes[$i] }
        $h = if ($sizes[$i] -ge 256) { 0 } else { [byte]$sizes[$i] }
        $len = [uint32]$pngBuffers[$i].Length

        $bw.Write([byte]$w)
        $bw.Write([byte]$h)
        $bw.Write([byte]0)   # Color count
        $bw.Write([byte]0)   # Reserved
        $bw.Write([uint16]1) # Planes
        $bw.Write([uint16]32)# BitCount
        $bw.Write($len)      # BytesInRes
        $bw.Write([uint32]$offset) # ImageOffset

        $offset += $len
    }

    # Write PNG payloads
    for ($i = 0; $i -lt $sizes.Count; $i++) {
        $bw.Write($pngBuffers[$i])
    }

    $bw.Flush()
    $bw.Close()
    $fs.Close()
}

function Save-Png([string]$outputPath, [int]$size = 256) {
    $bmp = Draw-TacticalSkull -size $size
    $bmp.Save($outputPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

$rootPath = Split-Path -Parent $PSScriptRoot
$icoRoot = Join-Path $rootPath "icon.ico"
$icoApp  = Join-Path $rootPath "App\icon.ico"
$icoInstaller = Join-Path $rootPath "Installer\icon.ico"
$pngRoot = Join-Path $rootPath "icon.png"
$pngApp  = Join-Path $rootPath "App\icon.png"

Save-MultiResIco -outputPath $icoRoot
Write-Output "Generado ICO exitosamente: $icoRoot"

Save-MultiResIco -outputPath $icoApp
Write-Output "Generado ICO exitosamente: $icoApp"

Save-MultiResIco -outputPath $icoInstaller
Write-Output "Generado ICO exitosamente: $icoInstaller"

Save-Png -outputPath $pngRoot -size 256
Write-Output "Generado PNG exitosamente: $pngRoot"

Save-Png -outputPath $pngApp -size 256
Write-Output "Generado PNG exitosamente: $pngApp"

# Genera mapas de normales (<nombre>_n.png) a partir de la luminancia de las
# texturas difusas que no tienen uno (altura suavizada + Sobel, bordes
# envueltos para texturas repetibles). El bucle de píxeles va en C# compilado
# con Add-Type; en PowerShell puro tardaría minutos por textura.
#   powershell -File tools/desktop/gen-normals.ps1 -List lista.txt -Root <packages> [-Strength 2.5] [-Force]
# lista.txt: una ruta relativa a packages por línea (p. ej. dg/mur059.jpg).
param(
    [Parameter(Mandatory=$true)][string]$List,
    [Parameter(Mandatory=$true)][string]$Root,
    [double]$Strength = 2.5,
    [switch]$Force
)
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public static class NormalGen {
    public static void Make(string src, string dst, double strength) {
        Bitmap img = (Bitmap)Image.FromFile(src);
        int w = img.Width, h = img.Height;
        Bitmap bmp = new Bitmap(w, h, PixelFormat.Format24bppRgb);
        using (Graphics g = Graphics.FromImage(bmp)) g.DrawImage(img, 0, 0, w, h);
        img.Dispose();
        Rectangle rect = new Rectangle(0, 0, w, h);
        BitmapData data = bmp.LockBits(rect, ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
        int stride = data.Stride;
        byte[] bytes = new byte[stride * h];
        Marshal.Copy(data.Scan0, bytes, 0, bytes.Length);
        bmp.UnlockBits(data); bmp.Dispose();
        double[] lum = new double[w * h];
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
            int i = y * stride + x * 3;
            lum[y * w + x] = 0.114 * bytes[i] + 0.587 * bytes[i + 1] + 0.299 * bytes[i + 2];
        }
        double[] sm = new double[w * h];
        for (int y = 0; y < h; y++) {
            int ym = (y - 1 + h) % h, yp = (y + 1) % h;
            for (int x = 0; x < w; x++) {
                int xm = (x - 1 + w) % w, xp = (x + 1) % w;
                sm[y * w + x] = (lum[ym*w+xm] + 2*lum[ym*w+x] + lum[ym*w+xp] + 2*lum[y*w+xm] + 4*lum[y*w+x] + 2*lum[y*w+xp] + lum[yp*w+xm] + 2*lum[yp*w+x] + lum[yp*w+xp]) / 16.0;
            }
        }
        byte[] outb = new byte[stride * h];
        double k = strength / 255.0;
        for (int y = 0; y < h; y++) {
            int ym = (y - 1 + h) % h, yp = (y + 1) % h;
            for (int x = 0; x < w; x++) {
                int xm = (x - 1 + w) % w, xp = (x + 1) % w;
                double dx = (sm[ym*w+xp] + 2*sm[y*w+xp] + sm[yp*w+xp]) - (sm[ym*w+xm] + 2*sm[y*w+xm] + sm[yp*w+xm]);
                double dy = (sm[yp*w+xm] + 2*sm[yp*w+x] + sm[yp*w+xp]) - (sm[ym*w+xm] + 2*sm[ym*w+x] + sm[ym*w+xp]);
                double nx = -dx * k, ny = dy * k, nz = 1.0;
                double len = Math.Sqrt(nx*nx + ny*ny + nz*nz);
                int i = y * stride + x * 3;
                outb[i + 2] = (byte)Math.Round((nx / len * 0.5 + 0.5) * 255);
                outb[i + 1] = (byte)Math.Round((ny / len * 0.5 + 0.5) * 255);
                outb[i]     = (byte)Math.Round((nz / len * 0.5 + 0.5) * 255);
            }
        }
        Bitmap res = new Bitmap(w, h, PixelFormat.Format24bppRgb);
        BitmapData d2 = res.LockBits(rect, ImageLockMode.WriteOnly, PixelFormat.Format24bppRgb);
        Marshal.Copy(outb, 0, d2.Scan0, outb.Length);
        res.UnlockBits(d2);
        res.Save(dst, ImageFormat.Png); res.Dispose();
    }
}
"@

$n = 0; $skip = 0; $missing = 0
foreach ($rel in Get-Content $List) {
    $rel = $rel.Trim(); if (-not $rel) { continue }
    $src = Join-Path $Root $rel
    if (-not (Test-Path $src)) { $missing++; continue }
    $dst = [System.IO.Path]::Combine([System.IO.Path]::GetDirectoryName($src), [System.IO.Path]::GetFileNameWithoutExtension($src) + "_n.png")
    if ((Test-Path $dst) -and -not $Force) { $skip++; continue }
    [NormalGen]::Make($src, $dst, $Strength)
    $n++
}
Write-Host "generados $n, ya existian $skip, sin fichero $missing"

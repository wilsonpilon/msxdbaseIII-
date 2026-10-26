# Gera CLIENTES.DBF (formato dBASE III, versao 0x03) para testes do Marco 1.
$out = Join-Path $PSScriptRoot "CLIENTES.DBF"

# nome, tipo, tamanho, decimais
$fields = @(
	@("NOME",  "C", 20, 0),
	@("IDADE", "N",  3, 0),
	@("SALDO", "N",  9, 2),
	@("ATIVO", "L",  1, 0),
	@("NASC",  "D",  8, 0)
)
# nome, idade, saldo, ativo, nascimento, excluido
$rows = @(
	@("Ana Souza",    "28", "1250.50", "T", "19970312", $false),
	@("Bruno Lima",   "41", " -80.00", "F", "19840730", $false),
	@("Carla Dias",   "35", "9999.99", "T", "19900101", $true),
	@("Diego Alves",  "19", "   0.00", "T", "20061225", $false),
	@("Elisa Rocha",  "63", " 310.75", "F", "19630504", $false)
)

$recLen = 1 + ($fields | ForEach-Object { $_[2] } | Measure-Object -Sum).Sum
$hdrLen = 32 + 32 * $fields.Count + 1
$ms = New-Object System.IO.MemoryStream
$w = New-Object System.IO.BinaryWriter($ms)

$w.Write([byte]3); $w.Write([byte]126); $w.Write([byte]10); $w.Write([byte]26)
$w.Write([uint32]$rows.Count); $w.Write([uint16]$hdrLen); $w.Write([uint16]$recLen)
$w.Write((New-Object byte[] 20))

foreach ($f in $fields) {
	$name = [System.Text.Encoding]::ASCII.GetBytes($f[0])
	$buf = New-Object byte[] 11
	[Array]::Copy($name, $buf, $name.Length)
	$w.Write($buf); $w.Write([byte][char]$f[1]); $w.Write((New-Object byte[] 4))
	$w.Write([byte]$f[2]); $w.Write([byte]$f[3]); $w.Write((New-Object byte[] 14))
}
$w.Write([byte]0x0D)

foreach ($r in $rows) {
	$w.Write([byte]$(if ($r[5]) { [char]'*' } else { [char]' ' }))
	for ($i = 0; $i -lt $fields.Count; $i++) {
		$len = $fields[$i][2]
		$val = $r[$i]
		$txt = if ($fields[$i][1] -eq "N") { $val.PadLeft($len) } else { $val.PadRight($len) }
		$w.Write([System.Text.Encoding]::ASCII.GetBytes($txt.Substring(0, $len)))
	}
}
$w.Write([byte]0x1A)
[System.IO.File]::WriteAllBytes($out, $ms.ToArray())
"Gerado: $out ($($ms.Length) bytes)"

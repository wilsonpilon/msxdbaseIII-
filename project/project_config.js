ProjName = "msxdbase";
ProjModules = [ "main", "console", "prompt", "commands", "platform", "dbf" ];
LibModules = [ "system", "bios", "vdp", "print", "input", "dos", "dos_mapper", "memory" ];
Machine = "2";
Target = "DOS2";
CheckVersion = false;
AppSignature = false;
CompileComplexity = "Default";
DiskSize = "720K";
DiskFiles = [ "data/CLIENTES.DBF", "data/VRAM.DAT" ];

if (process.platform === "win32")
	DskTool = `${ToolsDir}build/msxtar/msxtar.exe`;

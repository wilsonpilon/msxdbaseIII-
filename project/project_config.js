ProjName = "msxdbase";
ProjModules = [ "main", "console", "prompt", "commands", "help", "platform", "dbf" ];
LibModules = [ "system", "bios", "vdp", "print", "input", "dos", "dos_mapper", "memory" ];
Machine = "2";
Target = "DOS2";
CheckVersion = false;
AppSignature = false;
CompileComplexity = "Default";
DiskSize = "720K";
DiskFiles = [ "data/CLIENTES.DBF", "../resource/PESSOAL.DBF", "../resource/ALUNOS.DBF" ];
const fs = require("fs");
const path = require("path");
const helpFile = "../resource/dbaseII/files/DB8SEMSG.TXT";
if (fs.existsSync(path.resolve(__dirname, helpFile)))
	DiskFiles.push(helpFile);

if (process.platform === "win32")
	DskTool = `${ToolsDir}build/msxtar/msxtar.exe`;

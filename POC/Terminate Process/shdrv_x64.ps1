Clear-Host
Write-Host

$ProcID = Start-Process Notepad -PassThru | select -ExpandProperty ID
#Terminate-KernelProcess -ProcID $ProcID -Driver shdrv

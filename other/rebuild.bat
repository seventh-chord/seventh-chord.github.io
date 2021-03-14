@echo OFF

set PANDOC=%~dp0pandoc\pandoc.exe

set source=%1
if defined source (
    echo Rebuilding %1
    %PANDOC% %source% -f markdown -t html --output %~dp1%~n1.html --standalone --css ../other/style.css --include-before-body="%~dp0header.html" --include-after-body="%~dp0footer.html" --highlight-style zenburn --mathjax
) else (
    echo No parameter passed, so rebuilding index.html instead
    %PANDOC% %~dp0index.md -f markdown -t html --output index.html --standalone --css other/style.css --include-after-body=%~dp0footer.html --highlight-style zenburn
)

@echo OFF

set PANDOC="pandoc-2.0.2\pandoc.exe"

for %%x in (posts_src\*.*) do (
    %PANDOC% %%x -f markdown -t html --output posts/%%~nx.html --standalone --css ../style.css --include-before-body=header.html --include-after-body=footer.html --highlight-style zenburn
)

REM     Build index.html without header
%PANDOC% index.md -f markdown -t html --output index.html --standalone --css style.css --include-after-body=footer.html --highlight-style zenburn

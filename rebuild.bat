@echo OFF

set PANDOC="pandoc-2.0.2\pandoc.exe"

REM Math stuff is only inserted if it is actually used in a document, so we can pass the
REM flags to all documents without bloating them unnecesarily.

for %%x in (posts_src\*.*) do (
    %PANDOC% %%x -f markdown -t html --output posts/%%~nx.html --standalone --css ../style.css --include-before-body=other/header.html --include-after-body=other/footer.html --highlight-style zenburn --mathjax
)

REM     Build index.html without header
%PANDOC% index.md -f markdown -t html --output index.html --standalone --css style.css --include-after-body=other/footer.html --highlight-style zenburn

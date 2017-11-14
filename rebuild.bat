@echo OFF

set PANDOC="pandoc-2.0.2\pandoc.exe"

for %%x in (
    custom_blender_export
    index
) do (
    %PANDOC% %%x.md -f markdown -t html --output site/%%x.html --standalone --css style.css --include-before-body=header.html --include-after-body=footer.html --highlight-style zenburn
)

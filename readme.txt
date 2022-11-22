Put midi.mid in this directory and run `nmake`.
A clearer implementation (but building larger binary) is in ./simple

To contain the pdf file and source repo, run `nmake heartbeat_all.pdf` (or `heartbeat.pdf` for a pdf+exe version). GhostScript, NASM and pdfLatex should be in the search path.

`.git` in the zip file should contain no objects, but is pullable.

Thanks to @AtelierRitz for the midi.
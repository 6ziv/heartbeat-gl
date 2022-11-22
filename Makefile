heartbeat.exe: main.cpp playmidi.cpp midi.h playmidi.h shaders.hpp
    cl main.cpp playmidi.cpp -I midifile\include -Iglatter\include -Feheartbeat -Zc:inline- -DUNICODE -DNDEBUG user32.lib gdi32.lib opengl32.lib /GL /O1 /MD /Qspectre /GS /link /LTCG
	
midi.h: midi.mid bin2c.exe
    bin2c midi.mid midi.h bin2c_midi_mid

bin2c.exe: bin2c.c
    cl bin2c.c -DNDEBUG -DUNICODE -Febin2c

pdf\stub.com: pdf\stub.asm
	cd pdf
	nasm stub.asm -f bin -o stub.com
	cd ..
	
pdf\doc.pdf: pdf\doc.tex
	cd pdf
	-del doc2.pdf
	pdflatex doc.tex
	ren doc.pdf doc2.pdf
	gswin32c -sDEVICE=pdfwrite -dCompatibilityLevel=1.5 -dPDFSETTINGS=/screen -dNOPAUSE -dQUIET -dBATCH -sOutputFile=doc.pdf doc2.pdf
	-del doc2.pdf
	-del *.aux
	-del *.log
	cd ..

clean: .phony
    -del *.exe
	-del *.pdf
	-del *.pdb
	-del midi.h
	-del repo.zip
	-del pdf\doc.zip
	-del pdf\stub.com
	-del pdf\pdf_tailing.bin
	-del pdf\pdf_tailing.h

.phony:  
   -del *.obj
   -del *.exe
   -del midi.h
   -del pdf\doc.pdf
   -del repo.zip
 
repo.zip: bin2c.c main.cpp Makefile playmidi.cpp playmidi.h readme.txt shaders.hpp pdf\doc.tex simple\simple.cpp .git\HEAD .git\config .git\objects .git\refs
	zip repo.zip bin2c.c main.cpp Makefile playmidi.cpp playmidi.h readme.txt shaders.hpp pdf\doc.tex simple\simple.cpp glatter midifile .git\HEAD .git\config .git\objects .git\refs

concat.exe: concat.c
	cl concat.c -I midifile\include -Iglatter\include -Feconcat -Zc:inline- -DUNICODE -DNDEBUG user32.lib gdi32.lib opengl32.lib /GL /O1 /MD /Qspectre /GS /link /LTCG
	
pdf\pdf_tailing.bin: pdf\doc.pdf concat.exe pdf\pdf_tailing_prefix.bin
	concat pdf\pdf_tailing_prefix.bin pdf\doc.pdf pdf\pdf_tailing.bin

pdf\pdf_tailing.h: pdf\pdf_tailing.bin bin2c.exe
	bin2c pdf\pdf_tailing.bin pdf\pdf_tailing.h pdf_tail_data

fix_pdf_length.exe: pdf\fix_pdf_length.c
	cl pdf\fix_pdf_length.c -I midifile\include -Iglatter\include -Fefix_pdf_length -Zc:inline- -DUNICODE -DNDEBUG user32.lib gdi32.lib opengl32.lib /GL /O1 /MD /Qspectre /GS /link /LTCG
	
heartbeat.pdf: main.cpp playmidi.cpp midi.h playmidi.h shaders.hpp pdf\pdf_tailing.c pdf\pdf_tailing.h pdf\stub.com fix_pdf_length.exe
	cl main.cpp playmidi.cpp pdf\pdf_tailing.c -I midifile\include -Iglatter\include -Feheartbeat_pdf -Zc:inline- -DUNICODE -DNDEBUG user32.lib gdi32.lib opengl32.lib /GL /O1 /MD /Qspectre /GS /link /LTCG /stub:pdf\stub.com	
	copy heartbeat_pdf.exe heartbeat.pdf
	-del *.obj	
	fix_pdf_length heartbeat.pdf

heartbeat_all.pdf: concat.exe heartbeat.pdf repo.zip
	concat.exe heartbeat.pdf repo.zip heartbeat_all.pdf
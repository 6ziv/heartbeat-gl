heartbeat.exe: main.cpp playmidi.cpp midi.h playmidi.h shaders.hpp
    cl *.cpp -I midifile\include -Iglatter\include -Feheartbeat -Zc:inline- -DUNICODE -DNDEBUG user32.lib gdi32.lib opengl32.lib /GL /O1 /MD /link /LTCG
	-del *.obj
	
midi.h: midi.mid bin2c.exe
    bin2c midi.mid midi.h bin2c_midi_mid

bin2c.exe: bin2c.c
    cl bin2c.c -DNDEBUG -DUNICODE -Febin2c
	
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
	-del midi.h

.phony:  
   -del *.obj
   -del *.exe
   -del midi.h
   -del pdf\doc.pdf
   -del repo.zip

 
repo.zip: bin2c.c main.cpp Makefile playmidi.cpp playmidi.h readme.txt shaders.hpp pdf\doc.tex simple\simple.cpp .git\HEAD .git\config .git\objects .git\refs
	zip repo.zip bin2c.c main.cpp Makefile playmidi.cpp playmidi.h readme.txt shaders.hpp pdf\doc.tex simple\simple.cpp glatter midifile .git\HEAD .git\config .git\objects .git\refs
	
heartbeat.pdf: main.cpp playmidi.cpp midi.h playmidi.h shaders.hpp pdf\doc.pdf repo.zip
	echo MZ>tmp_stub.exe
	type pdf\doc.pdf>>tmp_stub.exe
	cl *.cpp -I midifile\include -Iglatter\include -Feheartbeat_pdf -Zc:inline- -DUNICODE -DNDEBUG user32.lib gdi32.lib opengl32.lib /GL /O1 /MD /link /LTCG /stub:tmp_stub.exe
	type heartbeat_pdf.exe repo.zip>heartbeat.pdf
	-del *.obj
	-del heartbeat_pdf.exe
	-del repo.zip
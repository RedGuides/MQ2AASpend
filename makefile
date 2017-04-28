!include "../global.mak"

ALL : "$(OUTDIR)\MQ2AASpend.dll"

CLEAN :
	-@erase "$(INTDIR)\MQ2AASpend.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\MQ2AASpend.dll"
	-@erase "$(OUTDIR)\MQ2AASpend.exp"
	-@erase "$(OUTDIR)\MQ2AASpend.lib"
	-@erase "$(OUTDIR)\MQ2AASpend.pdb"


LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib $(DETLIB) ..\Release\MQ2Main.lib /nologo /dll /incremental:no /pdb:"$(OUTDIR)\MQ2AASpend.pdb" /debug /machine:I386 /out:"$(OUTDIR)\MQ2AASpend.dll" /implib:"$(OUTDIR)\MQ2AASpend.lib" /OPT:NOICF /OPT:NOREF 
LINK32_OBJS= \
	"$(INTDIR)\MQ2AASpend.obj" \
	"$(OUTDIR)\MQ2Main.lib"

"$(OUTDIR)\MQ2AASpend.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) $(LINK32_FLAGS) $(LINK32_OBJS)


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("MQ2AASpend.dep")
!INCLUDE "MQ2AASpend.dep"
!ELSE 
!MESSAGE Warning: cannot find "MQ2AASpend.dep"
!ENDIF 
!ENDIF 


SOURCE=.\MQ2AASpend.cpp

"$(INTDIR)\MQ2AASpend.obj" : $(SOURCE) "$(INTDIR)"


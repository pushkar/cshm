TARGET = achshm	
DIRTARGET = libgolemachshm.a
TYPE = debug
DIRS = .
LIBS = rt pthread
DLIBS = 

ifeq ($(TYPE), debug)
LDPARAM = 
CCPARAM = -Wall -g
MACROS = 
endif

ifeq ($(TYPE), release)
LDPARAM = -s
CCPARAM = -Wall 
MACROS = NDEBUG
endif

INCPATH = .
LIBPATH = 

CXX = g++

STORE = $(TYPE)
SOURCE := $(foreach DIR,$(DIRS),$(wildcard $(DIR)/*.cpp))
HEADERS := $(foreach DIR,$(DIRS),$(wildcard $(DIR)/*.h))
OBJECTS := $(addprefix $(STORE)/, $(SOURCE:.cpp=.o))
DFILES := $(addprefix $(STORE)/,$(SOURCE:.cpp=.d))
AFILES := $(addprefix $(STORE)/,$(SOURCE:.cpp=.a))

.PHONY = clean backup dirs

all			 : $(DIRTARGET)

$(DIRTARGET) : dirs $(OBJECTS)
	@echo Creating library
	ar -rc -o $(DIRTARGET) $(OBJECTS)

$(TARGET): dirs $(OBJECTS)
	@echo Linking $(TARGET)
	@$(CXX) -o $(TARGET) $(OBJECTS) $(LDPARAM) $(foreach LIBRARY, $(LIBS),-l$(LIBRARY)) $(foreach LIB,$(LIBPATH),-L$(LIB))

$(STORE)/%.o: %.cpp
	@echo Creating object file for $*...
	@$(CC) -Wp,-MMD,$(STORE)/$*.dd $(CCPARAM) $(foreach INC,$(INCPATH),-I$(INC)) $(foreach MACRO,$(MACROS),-D$(MACRO)) -c $< -o $@
	@sed -e '1s/^\(.*\)$$/$(subst /,\/,$(dir $@))\1/' $(STORE)/$*.dd > $(STORE)/$*.d
	@rm -f $(STORE)/$*.dd
        
clean:
	@-rm -f $(foreach DIR,$(DIRS),$(STORE)/$(DIR)/*.d $(STORE)/$(DIR)/*.o)
	@-rm -f $(TARGET)
	@-rm -f $(DIRTARGET)
	@echo Cleaning Done

distclean:
	@-rm -f $(foreach DIR,$(DIRS),$(STORE)/$(DIR)/*.d $(STORE)/$(DIR)/*.o)
	@-rm -f $(TARGET)
	@-rm -f $(DIRTARGET)
	@-rm -rf debug/*
	@@-if [ -e debug ]; then rmdir debug; fi;
	@-rm -rf man/*
	@@-if [ -e man ]; then rmdir man; fi;
	@echo Cleaning Done

	
install:
	cp *.h $(DESTDIR)/usr/local/include/
	cp *.a $(DESTDIR)/usr/local/lib/
	
doc:
	@doxygen doxygen.config
	@cp ./man/man3/* $(DESTDIR)/usr/share/man/man3
        
backup:
	@-if [ ! -e backup ]; then mkdir backup; fi;
	@zip backup/backup_`date +%d-%m-%y_%H.%M`.zip $(SOURCE) $(HEADERS) $(EXTRA_FILES)
	@echo Backup created in .backup/backup_`date +%d-%m-%y_%H.%M`.zip

dirs:
	@-if [ ! -e $(STORE) ]; then mkdir $(STORE); fi;
	@-$(foreach DIR,$(DIRS), if [ ! -e $(STORE)/$(DIR) ]; then mkdir $(STORE)/$(DIR); fi; )

deb:
	@dpkg-buildpackage -ai386 -b -us -uc

-include $(DFILES)


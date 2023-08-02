# Template makefile by Michael Pyne.  This is public domain
#
# Intended for use with gcc and KDE.
#
# WARNING:  This Makefile is an
# absolute orgy of non-portability.  You should use this Makefile to aid
# in your development, but use a different build system to actually make
# releases when possible.

# This is the name of the executable to generate
TARGET = kfile_spc

# Version number of the program, useful for the make dist command
VERSION = 0.5.1

# This is a listing of all of the .cpp files that you've written.  Don't
# include automatically generated ones (like from the .ui files), and don't
# leave this blank.
SOURCES = kfile_spc.cpp

# This is a listing of all of the .kcfgc files that you've written that need to
# be compiled into the program.  This makefile will automatically generate the
# corresponding .h and .cpp files, so there is no need to list the .cpp file in
# SOURCES. Note that it's assumed you have at most one file, because I'm pretty
# sure that having two or more in the same directory makes no sense.
KCFGC_FILE =

# This is a list of all of the icons that need to be installed.  See the
# Makefile.am howto on developer.kde.org to find out how to name your icons so
# that they can be installed appropriately.  I recommend installing into the
# hicolor directory, as users of other themes will then pickup the icon
# automatically.
ICONS =

# This is a list of all of the .ui files that need to be uic'ed
# If you leave this blank, all .ui files in the current directory
# will be uic'ed
FORMS =

# This is a list of all of the .moc files that need to be generated.
# You shouldn't need to explicitly specify the moc files for files that
# #include them, or for the uic files.  Other types of moc files need
# to be listed here.
MOCS =

# This is a list of .desktop files to install to the XDG application data
# directory.
DESKTOP_FILES = kfile_spc.desktop

# This is a list of XMLGUI files that need installed.  Tend to take the name
# $(TARGET)ui.rc
XMLGUI_FILES =

# This is a list of mimetypes to install.  This is currently quite limited.
MIME_FILES = x-spc.desktop

# Change this to change the CPU that gcc compiles against.
CPU = i486

# These flags are passed to gcc for every c++ compilation.  Don't forget to
# change these to reasonable defaults for every machine when you make a
# release.
CXXFLAGS = -g -march=$(CPU) -pipe -Wall

# These flags are passed to the linker when linking the target.  You may
# want to add --as-needed if your version of binutils supports it.
LDFLAGS = -O1 --as-needed

# Add include directories you need here
OTHER_INCLUDES =

# Add non KDE libraries here
OTHER_LIBS =

# Add required KDE libraries here.  For example, -lkio
KDE_LIBS = -lkio

#
# You shouldn't have to change anything below this section.  If you do, please
# contact the author, Michael Pyne, at michael.pyne@kdemail.net
#

# Command to compile c++ files
CXX = g++

# Qt moc program
MOC = moc

# Qt uic program
UIC = uic

# Destination directory root.  Normally simply just /
DESTDIR = 

# Icon install buddy, needs to be distributed with this Makefile for this to
# work.
ICON_INSTALL_BUDDY = icon-install-buddy.pl

# KDE kconfig_compiler program
KCONFIG_COMPILER = kconfig_compiler

# Prefix for a KDE installation.  Only used to determine the include files
# directory.
KDE_PREFIX := `kde-config --expandvars --prefix`

# Directory to install the .kcfg files into
KCFG_DIR := `kde-config --expandvars --install kcfg`

# Directory to install the executable into
KDE_BINDIR := `kde-config --expandvars --install exe`

KDE_SERVICESDIR := `kde-config --expandvars --install services`

KDE_MODULEDIR := `kde-config --expandvars --install module`

# Directory that contains the .desktop files for the program
XDG_APP_DATADIR := `kde-config --expandvars --install xdgdata-apps`

# Directory holding mimetype definitions, not including the mime type category
# (e.g. audio, application, etc).
TARGET_MIMEDIR := `kde-config --expandvars --install mime`

# Directory holding the application's data
TARGET_DATADIR := `kde-config --expandvars --install data`/$(TARGET)

# Directory holding the Qt includes.  Change this if you have a non-standard
# Qt installation.
QT_INCLUDES = -I$(QTDIR)/include

# Directory holding the KDE includes.  Change this if you have a non-standard
# KDE installation.
KDE_INCLUDES = -I$(KDE_PREFIX)/include -I$(KDE_PREFIX)/include/kde

INCLUDES = $(QT_INCLUDES) $(KDE_INCLUDES) $(OTHER_INCLUDES)

LIBS = -Wl,'$(LDFLAGS)' -L$(QTDIR)/lib -L$(KDEDIR)/lib -lqt-mt -lkdecore \
       -lkdeui $(OTHER_LIBS) $(KDE_LIBS)

# Automatically detect moc files that need to be generated based on the include
# statements of the .cpp source.
AUTO_MOCS := $(basename $(shell grep -l '^ *\# *include *".*\.moc" *$$' $(SOURCES)))

# All moc files.  sort is used in order to remove duplicates.
ALL_MOCS = $(sort $(AUTO_MOCS) $(MOCS))

ALL_CXXFLAGS = $(CXXFLAGS) $(INCLUDES) -fPIC -Wno-non-virtual-dtor

# Used later to automatically include auto-generated files on the
# distclean list.
ALL_CLEAN =

# Used provided helper install script.
INSTALL = ./install.pl

# These targets don't reference an explicit file, make this clear to make
.PHONY: clean distclean all install uninstall

# These suffixes have implicit rules
.SUFFIXES: .o .moc .cpp .h .d

# Autodetect the FORMS if the user hasn't specified any.
ifeq ($(FORMS),)
FORMS=$(wildcard *.ui)
endif

UI_HEADERS = $(addsuffix .h,$(basename $(FORMS)))
UI_OBJS = $(addsuffix .o,$(basename $(FORMS)))
UI_CPPS = $(addsuffix .cpp,$(basename $(FORMS)))

ALL_CPPS = $(UI_CPPS) $(SOURCES)
DEPENDENCIES = $(addprefix .,$(ALL_CPPS:%.cpp=%.d))

OBJS = $(addsuffix .o,$(basename $(ALL_CPPS)))

all: $(TARGET).so depend

# Strips the generated executable
strip: all
	@echo -e "Stripping   $(TARGET)"
	@strip --strip-unneeded $(TARGET)

# The dependencies are automatically generated using a feature of gcc, the -MM
# switch, that outputs a Makefile suitable for managing the dependencies of the
# files being compiled.  The output is slightly altered to also make the
# .d file be regenerated whenever the .o file would change.
depend: $(DEPENDENCIES)

# Include dependency information, must come after first build rule to avoid a
# random build target.  Don't include dependency info if all we're doing is
# cleaning.
ifeq ($(filter dist clean distclean,$(MAKECMDGOALS)),)
-include $(DEPENDENCIES)
endif

# This is used later to automatically add dependencies to the Makefile for
# all of the .ui files
define MAKEdep
$(1).cpp : $(1).h $(1).ui

# Automatically add the moc file for the .ui to the build
OBJS += $(1)_moc.o

# Find files that rely on the .h being generated from the .ui, and make sure
# that their .d files depend on the .h being generated as well.
$(1)_LIST := $$(shell grep -s -l '^ *\# *include *"$(1)\.h" *$$$$' $(SOURCES) $(SOURCES:.cpp=.h))
$(1)_DEPENDS = $$(basename $$($(1)_LIST))
$$(addprefix .,$$(addsuffix .d, $$($(1)_DEPENDS))): $(1).h

# Prevent make from removing these files after building.  make might do this
# if the files were created as implicit links in the dependency chain.
.SECONDARY: $(1).cpp $(1).h $(1)_moc.cpp

# Make sure these files are cleaned after a make distclean
ALL_CLEAN += $(1).cpp $(1).h $(1)_moc.cpp
endef

# Make files that include a .moc file update after the .moc file updates.
define MAKEmocdep
$(1).cpp : $(1).moc $(1).h
endef

define MAKEkcfgcdep
$(1).h $(1).cpp : $(1).kcfgc

SOURCES += $(1).cpp
ALL_CLEAN += $(1).cpp $(1).h
endef

# Add dependencies for the .kcfgc file (if any)
$(eval $(call MAKEkcfgcdep,$(basename $(KCFGC_FILE))))

# Add dependencies for all of the .ui files
$(foreach ui,$(FORMS),$(eval $(call MAKEdep,$(basename $(ui)))))

# Now add the dependencies for all of the .moc files, both those that
# are user-specified, and those that are auto-detected.
$(foreach moc,$(ALL_MOCS),$(eval $(call MAKEmocdep,$(basename $(moc)))))

# Rule for creating the target from all of the objs that are required.
$(TARGET).so: $(OBJS)
	@echo -e "Linking     $(TARGET).so"
	@$(CXX) $(ALL_CXXFLAGS) $(LIBS) -o $(TARGET).so -shared $(OBJS)

# Default rules to create a .o from a .cpp file
%.o: %.cpp
	@echo -e "Compiling   $@"
	@$(CXX) $(ALL_CXXFLAGS) -c -o $@ $<

# Default rule to create a .h from a .ui file
%.h: %.ui
	@echo -e "Generating  $@"
	@$(UIC) -o $@ $<

# Default rule to create a skeleton header and class from a .kcfgc file.
%.h %.cpp: %.kcfgc $(TARGET).kcfg
	@echo -e "Generating  $*.cpp"
	@$(KCONFIG_COMPILER) -d . $(TARGET).kcfg $<

# Default rule to create a .cpp from a .h and .ui file.  You must make
# sure your build rules ensure the .h file is generated before the .cpp
# file is supposed to be generated.
%.cpp: %.ui
	@echo -e "Generating  $@"
	@$(UIC) -o $@ -impl $*.h $<

# Default rule to create a moc file from a .h file.  Used to generate the
# moc files for uic-generated cpp files.
%_moc.cpp: %.h
	@$(MOC) -o $@ $<

# Default rule to create a moc file from a .h file.
%.moc: %.h
	@echo -e "Generating  $@"
	@$(MOC) -o $@ $<

# Rule to update the dependency information for a .cpp file.  See the gmake
# texinfo documentation for more information.
.%.d: %.cpp
#	@echo -e "Scanning    $@"
	@$(CXX) -MG -MM $(ALL_CXXFLAGS) $< | sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' > $@

# Rule to clean the various generated files.
clean:
	@echo "Cleaning intermediate object files"
	@rm -f *.o *.gch *.moc .*.d
	@rm -f $(ALL_CLEAN)

# Rule to clean the various generated files, and also the target
distclean: clean
	@echo "Cleaning generated libraries"
	@rm -f $(TARGET).so

# Very basic rule to install the program.
install: all install-desktop-files install-mimetypes
	@$(INSTALL) $(TARGET).so $(DESTDIR)$(KDE_MODULEDIR)

# Installs the KConfigXT config file
install-kcfgs:
	@$(INSTALL) $(TARGET).kcfg $(DESTDIR)$(KCFG_DIR)

# Unfortunately this is not generic, the mimetypes must be installed into
# different directories depending on their category.
install-mimetypes:
	@$(INSTALL) $(MIME_FILES) $(DESTDIR)$(TARGET_MIMEDIR)/audio

# Installs the XMLGUI fooui.rc files
install-xmlgui-files:
	@$(INSTALL) $(XMLGUI_FILES) $(DESTDIR)$(TARGET_DATADIR)

# You should distribute the icon-install-buddy.pl script with your application.
install-icons:
	@./$(ICON_INSTALL_BUDDY) --install $(DESTDIR)$(ICONS)

# Installs the .desktop files.
install-desktop-files:
	@$(INSTALL) $(DESKTOP_FILES) $(DESTDIR)$(KDE_SERVICESDIR)

# Try to be encompassing enough without accidentally including stray .bz2 or .o
# files.
DISTFILES= Makefile $(ICONS) $(SOURCES) $(DESKTOP_FILES) \
           $(FORMS) $(MIME_FILES) $(KCFGC_FILE) *.h $(TARGET).kcfg \
	   $(XMLGUI_FILES) AUTHORS COPYING INSTALL README TODO spc*.txt \
	   install.pl

# This rule tries to create a .bz2 package of the program.
dist: distclean
	@[ -e $(TARGET)-$(VERSION) ] &&\
	    ( echo "Directory for the tar.bz2 already exists, and it shouldn't" ;\
	    false ) || true
	mkdir $(TARGET)-$(VERSION)
	cp $(DISTFILES) $(TARGET)-$(VERSION) 2>/dev/null; true
	tar cjf $(TARGET)-$(VERSION).tar.bz2 $(TARGET)-$(VERSION)
	rm -r $(TARGET)-$(VERSION)

# Very basic rule to uninstall the program.
uninstall:
	rm -f $(DESTDIR)$(KDE_MODULEDIR)/$(TARGET).so
	rm -f $(addprefix $(DESTDIR)$(KDE_SERVICESDIR)/,$(DESKTOP_FILES))
	rm -f $(addprefix $(DESTDIR)$(TARGET_MIMEDIR)/audio/,$(MIME_FILES))

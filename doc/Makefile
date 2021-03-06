#==============================================================================#
#
# Centralized build system for "doc".  
#
# see for instructions: http://puredata.org/docs/developer/build
#  <hans@at.or.at>
#
#==============================================================================#

CWD := $(shell pwd)

# these are designed to be overridden by the packages/Makefile
cvs_root_dir := $(shell cd $(CWD)/.. && pwd)
DESTDIR = $(CWD)/build/
BUILDLAYOUT_DIR = $(cvs_root_dir)/packages

CURL := curl --connect-timeout 600 --max-time 3600

# default target
all:
	@echo "this currently does nothing"

include $(BUILDLAYOUT_DIR)/Makefile.buildlayout

#==============================================================================#
#
# OVERARCHING BUILD TARGETS
#
#==============================================================================#

DOC_TARGETS = media messageoddness pd_fileformat pd-msg pddp \
tutorials

# clean up after everything is installed
final_setup:
	chmod -R ugo-w $(pddocdir)

install: $(objectsdir) $(helpdir) $(manualsdir) $(examplesdir) \
$(patsubst %, %_install,$(DOC_TARGETS))
	@echo " "
	@echo "doc install succeeded!"


#==============================================================================#
#
# PROJECT-SPECIFIC TARGETS
#
#==============================================================================#


#------------------------------------------------------------------------------#
# TEMPLATE
TEMPLATE_NAME = template
template_install: $(manualsdir)
	install -d $(helpdir)$(manualsdir)/$(TEMPLATE_NAME)
	install -p $(doc_src)/template/*.* \
		$(helpdir)$(manualsdir)/$(TEMPLATE_NAME) 

template_clean:
	-rm -f -- $(helpdir)$(manualsdir)/$(TEMPLATE_NAME)/*.*
	-rmdir --	$(helpdir)$(manualsdir)/$(TEMPLATE_NAME) 




#------------------------------------------------------------------------------#
# EXTERNALS-HOWTO
EXTERNALS-HOWTO_NAME = Externals-HOWTO
externals-howto_install: $(manualsdir)
	install -d $(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/pd-externals-HOWTO.pdf > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/pd-externals-HOWTO.pdf
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/HOWTO-externals-en.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/HOWTO-externals-en.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node1.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node1.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node2.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node2.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node3.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node3.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node4.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node4.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node5.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node5.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node6.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node6.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node7.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node7.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node8.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node8.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node9.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node9.html
	-$(CURL) http://iem.kug.ac.at/pd/externals-HOWTO/node10.html > \
		$(DESTDIR)$(manualsdir)/$(EXTERNALS-HOWTO_NAME)/node10.html

externals-howto_clean:




#------------------------------------------------------------------------------#
# MEDIA
MEDIA_NAME = media
media_install: $(pddocdir)
	install -d $(DESTDIR)$(pddocdir)/$(MEDIA_NAME)
	install -p $(doc_src)/media/*.* \
		$(DESTDIR)$(pddocdir)/$(MEDIA_NAME) 

media_clean:
	-rm -f -- $(DESTDIR)$(pddocdir)/$(MEDIA_NAME)/*.*
	-rmdir --	$(DESTDIR)$(pddocdir)/$(MEDIA_NAME) 




#------------------------------------------------------------------------------#
# MESSAGEODDNESS
MESSAGEODDNESS_NAME = MessageOddness
messageoddness_install: $(manualsdir)
	install -d $(DESTDIR)$(manualsdir)/$(MESSAGEODDNESS_NAME)
	install -p $(doc_src)/additional/messageoddness/*.* \
		$(DESTDIR)$(manualsdir)/$(MESSAGEODDNESS_NAME) 

messageoddness_clean:
	-rm -f -- $(DESTDIR)$(manualsdir)/$(MESSAGEODDNESS_NAME)/*.*
	-rmdir --	$(DESTDIR)$(manualsdir)/$(MESSAGEODDNESS_NAME) 




#------------------------------------------------------------------------------#
# PD-MSG
PD-MSG_NAME = pd-msg
pd-msg_install: $(manualsdir)
	install -d $(DESTDIR)$(manualsdir)/$(PD-MSG_NAME)
	install -p $(doc_src)/additional/pd-msg/*.txt \
		$(DESTDIR)$(manualsdir)/$(PD-MSG_NAME) 
	for dir in $(shell cd  $(doc_src)/additional/pd-msg && ls -d [1-5].*); do \
		echo "Including $$dir in $(PD-MSG_NAME)"; \
		install -d $(DESTDIR)$(manualsdir)/$(PD-MSG_NAME)/$$dir; \
		install -p $(doc_src)/additional/pd-msg/$$dir/*.* \
			$(DESTDIR)$(manualsdir)/$(PD-MSG_NAME)/$$dir; \
	done

pd-msg_clean:


#------------------------------------------------------------------------------#
# PD_FILEFORMAT
PD_FILEFORMAT_NAME = Pd
pd_fileformat_install: $(manualsdir)
	install -d $(DESTDIR)$(manualsdir)/$(PD_FILEFORMAT_NAME)
	install -p $(doc_src)/additional/pd-fileformat.html \
		$(DESTDIR)$(manualsdir)/$(PD_FILEFORMAT_NAME)/Pd_File_Format.html

pd_fileformat_clean:


#------------------------------------------------------------------------------#
# PDDP
pddp_install: $(helpdir)
	install -p $(doc_src)/pddp/*.pd $(DESTDIR)$(helpdir)
# this file is used in key-help.pd
	install -p $(doc_src)/pddp/glossary.txt $(DESTDIR)$(helpdir)
	install -p $(doc_src)/pddp/keyboard_fun.txt $(DESTDIR)$(helpdir)

pddp_clean:




#------------------------------------------------------------------------------#
# TUTORIALS
tutorials_install: $(manualsdir)
# start-here
	install -d "$(DESTDIR)$(manualsdir)/+ Start Here"
	install -p $(doc_src)/tutorials/start-here/*.* "$(DESTDIR)$(manualsdir)/+ Start Here"
# pddrums
	install -d $(DESTDIR)$(manualsdir)/PdDrums
	install -p $(doc_src)/tutorials/footils/pddrums/*.* $(DESTDIR)$(manualsdir)/PdDrums
# playnow
	install -d $(DESTDIR)$(manualsdir)/PlayNow
	install -p $(doc_src)/tutorials/playnow/*.* $(DESTDIR)$(manualsdir)/PlayNow
# cognition
	install -d $(DESTDIR)$(manualsdir)/Cognition
	install -p $(doc_src)/tutorials/cognition/*.* $(DESTDIR)$(manualsdir)/Cognition
# intro
	install -d $(DESTDIR)$(manualsdir)/0.Intro
	install -p $(doc_src)/tutorials/intro/*.* $(DESTDIR)$(manualsdir)/0.Intro
	install -p $(externals_src)/pddp/+pager.pd $(DESTDIR)$(manualsdir)/0.Intro
# sound
	install -d $(DESTDIR)$(manualsdir)/1.Sound
	install -p $(doc_src)/tutorials/sound/*.* $(DESTDIR)$(manualsdir)/1.Sound
	install -p $(externals_src)/pddp/+pager.pd $(DESTDIR)$(manualsdir)/1.Sound
# visual
	install -d $(DESTDIR)$(manualsdir)/2.Image
	install -p $(doc_src)/tutorials/visual/*.* $(DESTDIR)$(manualsdir)/2.Image
	install -p $(externals_src)/pddp/+pager.pd $(DESTDIR)$(manualsdir)/2.Image
# networking
	install -d $(DESTDIR)$(manualsdir)/3.Networking
	install -p $(doc_src)/tutorials/networking/*.* $(DESTDIR)$(manualsdir)/3.Networking
	install -p $(externals_src)/pddp/+pager.pd $(DESTDIR)$(manualsdir)/3.Networking
# physical
#	install -d $(manualsdir)/4.Physical
#	install -p $(doc_src)/tutorials/physical/*.* $(manualsdir)/4.Physical
#	install -p $(externals_src)/pddp/+pager.pd $(manualsdir)/4.Physical

tutorials_clean:
	-rm -f -- $(DESTDIR)$(manualsdir)/PdDrums/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/PdDrums
	-rm -f -- $(DESTDIR)$(manualsdir)/PlayNow/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/PlayNow
	-rm -f -- $(DESTDIR)$(manualsdir)/0.Intro/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/0.Intro
	-rm -f -- $(DESTDIR)$(manualsdir)/1.Sound/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/1.Sound
	-rm -f -- $(DESTDIR)$(manualsdir)/0.Intro/Visual/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/0.Intro/Visual
	-rm -f -- $(DESTDIR)$(manualsdir)/3.Networking/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/3.Networking
	-rm -f -- $(DESTDIR)$(manualsdir)/4.Physical/*.*
	-rmdir -- $(DESTDIR)$(manualsdir)/4.Physical


#==============================================================================#
#
# DEVELOPER'S TARGETS
#
#==============================================================================#

# make the symlinks necessary to simulate the installed environment
devsymlinks:
	test -e $(doc_src)/2.control.examples || \
		ln -s $(pd_src)/doc/2.control.examples $(doc_src)/2.control.examples
	test -e $(doc_src)/3.audio.examples || \
		ln -s $(pd_src)/doc/3.audio.examples $(doc_src)/3.audio.examples
# pd <= 0.38-4 has 4.fft.examples
	test -e $(doc_src)/4.fft.examples || \
		ln -s $(pd_src)/doc/4.fft.examples $(doc_src)/4.fft.examples
# pd > 0.38-4 has 4.data.structures
	test -e $(doc_src)/4.data.structures || \
		ln -s $(pd_src)/doc/4.data.structures $(doc_src)/4.data.structures
	test -e $(doc_src)/7.stuff || \
		ln -s $(pd_src)/doc/7.stuff $(doc_src)/7.stuff
	test -e $(doc_src)/sound || \
		ln -s $(pd_src)/doc/sound $(doc_src)/sound
# put the pddp support objects in the path
	test -e $(doc_src)/pddp/pddp || \
		ln -s $(externals_src)/pddp $(doc_src)/pddp/pddp

#==============================================================================#
#
# CLEAN TARGETS
#
#==============================================================================#

# the destination-specific clean targets are in Makefile.buildlayout
clean: $(patsubst %, %_clean,$(DOC_TARGETS))

distclean: clean cruft_clean




test_locations:
	@echo "PD_VERSION: $(PD_VERSION)"
	@echo "PACKAGE_VERSION: $(PACKAGE_VERSION)"
	@echo "CWD $(CWD)"
	@echo "DESTDIR $(DESTDIR)"
	@echo "PREFIX $(prefix)"
	@echo "BINDIR  $(bindir)"
	@echo "LIBDIR  $(libdir)"
	@echo "OBJECTSDIR  $(objectsdir)"
	@echo "PDDOCDIR  $(pddocdir)"
	@echo "LIBPDDIR  $(libpddir)"
	@echo "LIBPDBINDIR  $(libpdbindir)"
	@echo "HELPDIR  $(helpdir)"
	@echo "MANUALSDIR  $(manualsdir)"
	@echo "EXAMPLESDIR  $(examplesdir)"

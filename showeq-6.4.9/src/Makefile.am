INCLUDES = $(QT_INCLUDES) $(X_INCLUDES) -DPKGDATADIR=\"$(pkgdatadir)/\" -I/usr/include/pcap $(USER_INCLUDES)

QT_LIBS = $(LIB_QT)

bin_PROGRAMS = showeq

showeq_SOURCES = \
				 bazaarlog.cpp \
				 category.cpp \
				 combatlog.cpp \
				 compass.cpp \
				 compassframe.cpp \
				 datalocationmgr.cpp \
				 datetimemgr.cpp \
				 diagnosticmessages.cpp \
				 editor.cpp \
				 eqstr.cpp \
				 experiencelog.cpp \
				 filter.cpp \
				 filteredspawnlog.cpp \
				 filterlistwindow.cpp \
				 filtermgr.cpp \
				 filternotifications.cpp \
				 group.cpp \
				 guild.cpp \
				 guildlist.cpp \
				 guildshell.cpp \
				 interface.cpp \
				 logger.cpp \
				 main.cpp \
				 mapcore.cpp \
				 map.cpp \
				 mapicon.cpp \
				 mapicondialog.cpp \
				 message.cpp \
				 messagefilter.cpp \
				 messagefilterdialog.cpp \
				 messages.cpp \
				 messageshell.cpp \
				 messagewindow.cpp \
				 netdiag.cpp \
				 netstream.cpp \
				 packetcapture.cpp \
				 packetcaptureprovider.cpp \
				 packet.cpp \
				 packetformat.cpp \
				 packetfragment.cpp \
				 packetinfo.cpp \
				 packetlog.cpp \
				 packetstream.cpp \
				 player.cpp \
				 seqlistview.cpp \
				 seqwindow.cpp \
				 skilllist.cpp \
				 spawn.cpp \
				 spawnlist2.cpp \
				 spawnlistcommon.cpp \
				 spawnlist.cpp \
				 spawnlog.cpp \
				 spawnmonitor.cpp \
				 spawnpointlist.cpp \
				 spawnshell.cpp \
				 spelllist.cpp \
				 spells.cpp \
				 spellshell.cpp \
				 statlist.cpp \
				 terminal.cpp \
				 toolbaricons.cpp \
				 util.cpp \
				 vpacket.cpp \
				 xmlconv.cpp \
				 xmlpreferences.cpp \
				 zonemgr.cpp


showeq_moc_SRCS = \
				  bazaarlog.moc \
				  category.moc \
				  combatlog.moc \
				  compassframe.moc \
				  compass.moc \
				  datetimemgr.moc \
				  editor.moc \
				  experiencelog.moc \
				  filteredspawnlog.moc \
				  filterlistwindow.moc \
				  filtermgr.moc \
				  filternotifications.moc \
				  group.moc \
				  guildlist.moc \
				  guild.moc \
				  guildshell.moc \
				  interface.moc \
				  logger.moc \
				  mapicondialog.moc \
				  mapicon.moc \
				  map.moc \
				  messagefilterdialog.moc \
				  messagefilter.moc \
				  messageshell.moc \
				  messages.moc \
				  messagewindow.moc \
				  netdiag.moc \
				  packetinfo.moc \
				  packetlog.moc \
				  packet.moc \
				  packetstream.moc \
				  player.moc \
				  seqlistview.moc \
				  seqwindow.moc \
				  skilllist.moc \
				  spawnlist2.moc \
				  spawnlistcommon.moc \
				  spawnlist.moc \
				  spawnlog.moc \
				  spawnmonitor.moc \
				  spawnpointlist.moc \
				  spawnshell.moc \
				  spelllist.moc \
				  spellshell.moc \
				  statlist.moc \
				  terminal.moc \
				  xmlpreferences.moc \
				  zonemgr.moc


nodist_showeq_SOURCES = ui_mapicondialog.h $(showeq_moc_SRCS)

$(srcdir)/bazaarlog.cpp: bazaarlog.moc
$(srcdir)/category.cpp: category.moc
$(srcdir)/combatlog.cpp: combatlog.moc
$(srcdir)/compass.cpp: compass.moc
$(srcdir)/compassframe.cpp: compassframe.moc
$(srcdir)/datetimemgr.cpp: datetimemgr.moc
$(srcdir)/editor.cpp: editor.moc
$(srcdir)/experiencelog.cpp: experiencelog.moc
$(srcdir)/filteredspawnlog.cpp: filteredspawnlog.moc
$(srcdir)/filterlistwindow.cpp: filterlistwindow.moc
$(srcdir)/filtermgr.cpp: filtermgr.moc
$(srcdir)/filternotifications.cpp: filternotifications.moc
$(srcdir)/group.cpp: group.moc
$(srcdir)/guild.cpp: guild.moc
$(srcdir)/guildlist.cpp: guildlist.moc
$(srcdir)/guildshell.cpp: guildshell.moc
$(srcdir)/interface.cpp: interface.moc
$(srcdir)/logger.cpp: logger.moc
$(srcdir)/map.cpp: map.moc
$(srcdir)/mapicon.cpp: mapicon.moc
$(srcdir)/mapicondialog.cpp: mapicondialog.moc
$(srcdir)/messagefilter.cpp: messagefilter.moc
$(srcdir)/messagefilterdialog.cpp: messagefilterdialog.moc
$(srcdir)/messages.cpp: messages.moc
$(srcdir)/messageshell.cpp: messageshell.moc
$(srcdir)/messagewindow.cpp: messagewindow.moc
$(srcdir)/netdiag.cpp: netdiag.moc
$(srcdir)/packet.cpp: packet.moc
$(srcdir)/packetinfo.cpp: packetinfo.moc
$(srcdir)/packetlog.cpp: packetlog.moc
$(srcdir)/packetstream.cpp: packetstream.moc
$(srcdir)/player.cpp: player.moc
$(srcdir)/seqlistview.cpp: seqlistview.moc
$(srcdir)/seqwindow.cpp: seqwindow.moc
$(srcdir)/skilllist.cpp: skilllist.moc
$(srcdir)/spawnlist.cpp: spawnlist.moc
$(srcdir)/spawnlist2.cpp: spawnlist2.moc
$(srcdir)/spawnlistcommon.cpp: spawnlistcommon.moc
$(srcdir)/spawnlog.cpp: spawnlog.moc
$(srcdir)/spawnmonitor.cpp: spawnmonitor.moc
$(srcdir)/spawnpointlist.cpp: spawnpointlist.moc
$(srcdir)/spawnshell.cpp: spawnshell.moc
$(srcdir)/spelllist.cpp: spelllist.moc
$(srcdir)/spellshell.cpp: spellshell.moc
$(srcdir)/statlist.cpp: statlist.moc
$(srcdir)/terminal.cpp: terminal.moc
$(srcdir)/xmlpreferences.cpp: xmlpreferences.moc
$(srcdir)/zonemgr.cpp: zonemgr.moc
$(srcdir)/packetinfo.cpp: s_everquest.h

showeq_LDADD = $(QT_LDFLAGS) $(QT_LIBS) $(LIBPTHREAD) $(MEMORY_LIBS) \
$(PROFILE_LIBS) $(SHOWEQ_RPATH) $(USER_LDFLAGS)

TEST_PROGS = sortitem

if CGI
if HAVE_GD
GD_CGI_PROGS = drawmap.cgi
endif

CGI_PROGS = $(GD_CGI_PROGS) listspawn.cgi showspawn.cgi
endif

noinst_PROGRAMS = $(TEST_PROGS) $(CGI_PROGS)

listspawn_cgi_SOURCES = listspawn.cpp spawn.cpp util.cpp diagnosticmessageslight.cpp cgiconv.cpp
nodist_listspawn_cgi_SOURCES = 
listspawn_cgi_LDADD = $(QT_LDFLAGS) $(QT_LIBS) $(LIBPTHREAD) $(SHOWEQ_RPATH) $(USER_LDFLAGS)

showspawn_cgi_SOURCES = showspawn.cpp spawn.cpp util.cpp diagnosticmessageslight.cpp cgiconv.cpp
nodist_showspawn_cgi_SOURCES =
showspawn_cgi_LDADD = $(QT_LDFLAGS) $(QT_LIBS) $(LIBPTHREAD) $(SHOWEQ_RPATH) $(USER_LDFLAGS)

drawmap_cgi_SOURCES = drawmap.cpp util.cpp diagnosticmessageslight.cpp cgiconv.cpp
nodist_drawmap_cgi_SOURCES = 
drawmap_cgi_LDADD = $(QT_LDFLAGS) $(QT_LIBS) -lgd $(LIBPTHREAD) $(SHOWEQ_RPATH) $(USER_LDFLAGS)

sortitem_SOURCES = sortitem.cpp util.cpp diagnosticmessageslight.cpp 
nodist_sortitem_SOURCES = 
sortitem_LDADD = $(QT_LDFLAGS) $(QT_LIBS) $(LIBPTHREAD) $(SHOWEQ_RPATH) $(USER_LDFLAGS)

EXTRA_DIST = h2info.pl

noinst_HEADERS = \
				 bazaarlog.h \
				 category.h \
				 cgiconv.h \
				 classes.h \
				 combatlog.h \
				 compassframe.h \
				 compass.h \
				 crctab.h \
				 datalocationmgr.h \
				 datetimemgr.h \
				 decode.h \
				 deity.h \
				 diagnosticmessages.h \
				 editor.h \
				 eqstr.h \
				 everquest.h \
				 experiencelog.h \
				 filteredspawnlog.h \
				 filter.h \
				 filterlistwindow.h \
				 filtermgr.h \
				 filternotifications.h \
				 fixpt.h \
				 group.h \
				 guild.h \
				 guildlist.h \
				 guildshell.h \
				 interface.h \
				 languages.h \
				 logger.h \
				 main.h \
				 mapcolors.h \
				 mapcore.h \
				 map.h \
				 mapicondialog.h \
				 mapicondialog.ui \
				 mapicon.h \
				 messagefilterdialog.h \
				 messagefilter.h \
				 message.h \
				 messages.h \
				 messageshell.h \
				 messagewindow.h \
				 netdiag.h \
				 netstream.h \
				 packetcapture.h \
				 packetcaptureprovider.h \
				 packetcommon.h \
				 packetformat.h \
				 packetfragment.h \
				 packet.h \
				 packetinfo.h \
				 packetlog.h \
				 packetstream.h \
				 player.h \
				 pointarray.h \
				 point.h \
				 races.h \
				 seqlistview.h \
				 seqwindow.h \
				 s_everquest.h \
				 skilllist.h \
				 skills.h \
				 spawn.h \
				 spawnlist2.h \
				 spawnlistcommon.h \
				 spawnlist.h \
				 spawnlog.h \
				 spawnmonitor.h \
				 spawnpointlist.h \
				 spawnshell.h \
				 spelllist.h \
				 spells.h \
				 spellshell.h \
				 staticspells.h \
				 statlist.h \
				 terminal.h \
				 toolbaricons.h \
				 typenames.h \
				 util.h \
				 vpacket.h \
				 weapons1.h \
				 weapons27.h \
				 weapons28.h \
				 weapons29.h \
				 weapons2a.h \
				 weapons2b.h \
				 weapons2c.h \
				 weapons2d.h \
				 weapons2e.h \
				 weapons2f.h \
				 weapons30.h \
				 weapons4e.h \
				 weapons.h \
				 xmlconv.h \
				 xmlpreferences.h \
				 zonemgr.h \
				 zones.h


CLEANFILES = $(nodist_showeq_SOURCES)

MAINTAINERCLEANFILES = Makefile.in s_everquest.h

map.o: map.cpp map.h ui_mapicondialog.h

.h.moc:
	$(MOC) $< -o $@

m_%.cpp: %.h
	$(MOC) $< -o $@

s_%.h: %.h
	$(srcdir)/h2info.pl $< $@

ui_%.h: %.ui
	$(UIC) $< > $@


if CGI
install-exec-local: $(CGI_PROGS)
	$(mkinstalldirs) $(CGIDIR)
	@list='$(CGI_PROGS)'; for p in $$list; do \
	if test -f $$p; then \
		echo " $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(CGIDIR)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
		$(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(CGIDIR)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
		else :; fi; \
	done
endif



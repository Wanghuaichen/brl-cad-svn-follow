set(MGED_INCLUDE_DIRS
  ${BRLCAD_BINARY_DIR}/include
  ${BRLCAD_SOURCE_DIR}/include
  ${TCLCAD_INCLUDE_DIRS}
  ${GED_INCLUDE_DIRS}
  ${WDB_INCLUDE_DIRS}
  ${DM_INCLUDE_DIRS}
  ${BU_INCLUDE_DIRS}
  ${NMG_INCLUDE_DIRS}
  ${TK_INCLUDE_PATH}
  ${TCL_INCLUDE_PATH}
  ${TERMIO_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

BRLCAD_INCLUDE_DIRS(MGED_INCLUDE_DIRS)

set(MGED_SOURCES
  adc.c
  arbs.c
  attach.c
  axes.c
  buttons.c
  chgmodel.c
  chgtree.c
  chgview.c
  cmd.c
  clone.c
  color_scheme.c
  columns.c
  dir.c
  dm-generic.c
  dodraw.c
  doevent.c
  dozoom.c
  edarb.c
  edars.c
  edpipe.c
  edsol.c
  facedef.c
  fbserv.c
  grid.c
  history.c
  mater.c
  menu.c
  mged.c
  mover.c
  muves.c
  overlay.c
  plot.c
  predictor.c
  rect.c
  rtif.c
  scroll.c
  set.c
  setup.c
  share.c
  tedit.c
  titles.c
  track.c
  update.c
  usepen.c
  utility1.c
  vparse.c
  wdb_obj.c
  )

set(mged_libs libtclcad libged libwdb libdm libnmg ${ITCL_LIBRARY} ${ITK_LIBRARY})
if(TARGET libtermio)
  set(mged_libs ${mged_libs} libtermio)
endif(TARGET libtermio)
if(CPP_DLL_DEFINES)
  set(mged_libs ${mged_libs} ws2_32.lib;opengl32.lib)
endif(CPP_DLL_DEFINES)

# Ideally we'd build as GUI for Windows to avoid the command window popping up,
# but I'm not sure if that would be a problem for scripting with mged.exe on
# Windows so leave it for now...
BRLCAD_ADDEXEC(mged "${MGED_SOURCES}" "${mged_libs}")

# mged depends on the creation/installation of all the
# tclscripts.
foreach(item ${tclindex_target_list})
  ADD_DEPENDENCIES(mged ${item})
endforeach(item ${tclindex_target_list})

BRLCAD_ADDEXEC(cad_boundp cad_boundp.c "libbu;${M_LIBRARY}")

BRLCAD_ADDEXEC(cad_parea cad_parea.c libbu)

set(mged_ignore_files
  bool_rewrite.c
  cmd.h
  comb_bool.h
  comb_bool_parse.y
  comb_bool_scan.l
  fbserv.h
  htmlLibraryUi.h
  menu.h
  mged.h
  mged_dm.h
  qray.h
  red.c
  scroll.h
  sedit.h
  titles.h
  )
CMAKEFILES(${mged_ignore_files})

# Local Variables:
# tab-width: 8
# mode: cmake
# indent-tabs-mode: t
# End:
# ex: shiftwidth=2 tabstop=8

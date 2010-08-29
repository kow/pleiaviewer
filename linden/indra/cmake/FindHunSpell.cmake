FIND_PATH(HUNSPELL_INCLUDE_DIR hunspell.hxx
  /usr/local/include/hunspell
  /usr/local/include
  /usr/include/hunspell
  /usr/include
  )

SET(HUNSPELL_NAMES ${HUNSPELL_NAMES} hunspell hunspell-1.2)
FIND_LIBRARY(HUNSPELL_LIBRARY
  NAMES ${HUNSPELL_NAMES}
  PATHS /usr/lib /usr/local/lib
  )

IF(HUNSPELL_LIBRARY AND HUNSPELL_INCLUDE_DIR)
  SET(HUNSPELL_LIBRARIES ${HUNSPELL_LIBRARY})
  SET(HUNSPELL_FOUND "YES")
ELSE(HUNSPELL_LIBRARY AND HUNSPELL_INCLUDE_DIR)
  SET(HUNSPELL_FOUND "NO")
ENDIF(HUNSPELL_LIBRARY AND HUNSPELL_INCLUDE_DIR)

IF(HUNSPELL_FOUND)
  IF(NOT HUNSPELL_FIND_QUIETLY)
    MESSAGE(STATUS "Found Hunspell: ${HUNSPELL_LIBRARIES}")
  ENDIF(NOT HUNSPELL_FIND_QUIETLY)
ELSE(HUNSPELL_FOUND)
  IF(HUNSPELL_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find HunSpell library")
  ENDIF(HUNSPELL_FIND_REQUIRED)
ENDIF(HUNSPELL_FOUND)

MARK_AS_ADVANCED(
  HUNSPELL_LIBRARY
  HUNSPELL_INCLUDE_DIR
  )

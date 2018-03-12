# - Try to find libsbml
# Once done, this will define
#
#  libsbml_FOUND - system has libsbml
#  libsbml_INCLUDE_DIRS - the libsbml include directories
#  libsbml_LIBRARIES - link these to use libsbml

include(LibFindMacros)

set(_sbml_hints
    ${SBML_ROOT}/include
    ${SBML_ROOT}/lib
    $ENV{SBML_ROOT}
    $ENV{HOME}/usr/include
    $ENV{HOME}/usr/lib
    /usr/local/include
    /usr/local/lib
    /usr/include
    /usr/lib
)

# Include dir
find_path(libsbml_INCLUDE_DIR
  NAMES sbml/SBMLTypes.h
  PATH_SUFFIXES sbml
  HINTS ${_sbml_hints}
)

# Finally the library itself
find_library(libsbml_LIBRARY
  NAMES sbml
  HINTS ${_sbml_hints}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(libsbml_PROCESS_INCLUDES libsbml_INCLUDE_DIR)
set(libsbml_PROCESS_LIBS libsbml_LIBRARY)
libfind_process(libsbml)

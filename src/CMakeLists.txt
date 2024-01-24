add_library(csdr-eti SHARED csdr-eti.cpp meta.cpp version.cpp dab_tables.c dab.c fic.c misc.c viterbi.c depuncture.c)
file(GLOB LIBCSDRETI_HEADERS
    "${PROJECT_SOURCE_DIR}/include/*.hpp"
    "${PROJECT_SOURCE_DIR}/include/*.h"
)
set_target_properties(csdr-eti PROPERTIES PUBLIC_HEADER "${LIBCSDRETI_HEADERS}")
target_link_libraries(csdr-eti Csdr::csdr ${FFTW3_LIBRARIES})
set_target_properties(csdr-eti PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
install(TARGETS csdr-eti
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/csdr-eti
)
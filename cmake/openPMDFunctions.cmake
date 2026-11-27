# Prints a summary of openPMD-api options at the end of the CMake configuration
#
function(openpmd_print_summary)
    message("")
    message("openPMD build configuration:")
    message("  library Version: ${openPMD_VERSION}")
    message("  openPMD Standard: ${openPMD_STANDARD_VERSION}")
    message("  C++ Compiler: ${CMAKE_CXX_COMPILER_ID} "
                            "${CMAKE_CXX_COMPILER_VERSION} "
                            "${CMAKE_CXX_COMPILER_WRAPPER}")
    message("    ${CMAKE_CXX_COMPILER}")
    message("")
    if(openPMD_INSTALL)
        message("  Install with RPATHs: ${openPMD_INSTALL_RPATH}")
        message("  Installation prefix: ${openPMD_INSTALL_PREFIX}")
        message("        bin: ${openPMD_INSTALL_BINDIR}")
        message("        lib: ${openPMD_INSTALL_LIBDIR}")
        message("    include: ${openPMD_INSTALL_INCLUDEDIR}")
        message("      cmake: ${openPMD_INSTALL_CMAKEDIR}")
        if(openPMD_HAVE_PYTHON)
            message("     python: ${openPMD_INSTALL_PYTHONDIR}")
        endif()
    else()
        message("  Installation: OFF")
    endif()
    message("")
    message("  Build Type: ${CMAKE_BUILD_TYPE}")
    if(openPMD_BUILD_SHARED_LIBS)
        message("  Library: shared")
    else()
        message("  Library: static")
    endif()
    message("  CLI Tools: ${openPMD_BUILD_CLI_TOOLS}")
    message("  Examples: ${openPMD_BUILD_EXAMPLES}")
    message("  Testing: ${openPMD_BUILD_TESTING}")
    message("  Invasive Tests: ${openPMD_USE_INVASIVE_TESTS}")
    message("  Internal VERIFY: ${openPMD_USE_VERIFY}")
    message("  Build Options:")

    foreach(opt IN LISTS openPMD_CONFIG_OPTIONS)
      if(${openPMD_HAVE_${opt}})
        message("    ${opt}: ON")
      else()
        message("    ${opt}: OFF")
      endif()
    endforeach()
    message("")
endfunction()


# Escapes a single argument for the Libs and Cflags fields of a pkg-config
# .pc file and stores it in the variable <outname>
#
function(openpmd_pc_escape_arg outname arg)
    # argument vectors are split shell-like and # starts a comment
    # (not handled: "${" and "$$" are subject to variable expansion)
    string(REGEX REPLACE "([\\\\\"' \t#])" "\\\\\\1" arg "${arg}")
    set(${outname} "${arg}" PARENT_SCOPE)
endfunction()


# Escapes arguments for the Libs and Cflags fields of a pkg-config .pc file
# and appends them, each prefixed by a space, to the variable <outname>
#
function(openpmd_pc_escape outname)
    set(escaped "${${outname}}")
    foreach(arg IN LISTS ARGN)
        if(arg STREQUAL "")
            continue()
        endif()
        openpmd_pc_escape_arg(arg "${arg}")
        string(APPEND escaped " ${arg}")
    endforeach()
    set(${outname} "${escaped}" PARENT_SCOPE)
endfunction()


# Appends link items, each prefixed by a space, to the variable <outname>
# for the Libs fields of a pkg-config .pc file:
# - shared libraries given by absolute path become -L<dir> -l<name>, since
#   importers of .pc files (e.g., CMake's FindPkgConfig) place plain paths
#   before the object files, which fails to link with --as-needed
# - other absolute paths are escaped
# - other items are appended as they are, since they can consist of
#   multiple arguments, e.g., "-framework Accelerate"
#
function(openpmd_pc_link_items outname)
    set(items "${${outname}}")
    foreach(item IN LISTS ARGN)
        if(item STREQUAL "")
            continue()
        elseif(IS_ABSOLUTE "${item}" AND
               item MATCHES "^(.+)/lib([^/]+)\\.(so|dylib)$")
            openpmd_pc_escape(items "-L${CMAKE_MATCH_1}" "-l${CMAKE_MATCH_2}")
        elseif(IS_ABSOLUTE "${item}")
            openpmd_pc_escape(items "${item}")
        else()
            string(APPEND items " ${item}")
        endif()
    endforeach()
    set(${outname} "${items}" PARENT_SCOPE)
endfunction()

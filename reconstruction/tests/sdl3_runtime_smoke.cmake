# Paquet local de validation, ressources jamais ajoutees au depot.
# Aucun outil Python, original executable ou overlay dans ce dossier.
if(NOT EXISTS "${TERA_RESOURCE_DIR}/NOMPERS" OR NOT EXISTS "${TERA_RESOURCE_DIR}/VALMON")
    message("SKIP: original NOMPERS/VALMON absent")
    return()
endif()
file(MAKE_DIRECTORY "${TERA_SMOKE_DIR}/data")
foreach(forbidden TERA.COM TERA.000)
    if(EXISTS "${TERA_SMOKE_DIR}/data/${forbidden}" OR EXISTS "${TERA_SMOKE_DIR}/${forbidden}")
        message(FATAL_ERROR "Runtime autonomy directory contains forbidden reference file: ${forbidden}")
    endif()
endforeach()
file(COPY "${TERA_SDL_EXE}" "${TERA_SDL_DLL}" DESTINATION "${TERA_SMOKE_DIR}")
foreach(resource NOMPERS VALMON VERBES OBJETS NOMON MESSAGE MESSAGE2 MESSAGE3
    CRAPIM.IMA ASTRON.IMA BANNIERE.IMA FACADE.IMA KRONO.IMA MAISONJ.IMA MAISONS.IMA)
    if(NOT EXISTS "${TERA_RESOURCE_DIR}/${resource}")
        message("SKIP: original resource ${resource} absent")
        return()
    endif()
    file(COPY "${TERA_RESOURCE_DIR}/${resource}" DESTINATION "${TERA_SMOKE_DIR}/data")
endforeach()
get_filename_component(exe_name "${TERA_SDL_EXE}" NAME)
execute_process(COMMAND "${CMAKE_COMMAND}" -E env
    "SDL_VIDEODRIVER=dummy" "SDL_RENDER_DRIVER=software"
    "PATH=$ENV{SystemRoot}/System32" --unset=PYTHONHOME --unset=PYTHONPATH
    "${TERA_SMOKE_DIR}/${exe_name}" --smoke --save smoke.sav --capture smoke.bmp
    WORKING_DIRECTORY "${TERA_SMOKE_DIR}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "SDL autonomous runtime failed (${result}): ${output}${error}")
endif()
message("${output}")

# Embedded deployment check: never copy or discover original resource files.
file(MAKE_DIRECTORY "${TERA_SMOKE_DIR}" "${TERA_SMOKE_DIR}/other-cwd")
file(GLOB contents "${TERA_SMOKE_DIR}/*")
foreach(path IN LISTS contents)
    get_filename_component(name "${path}" NAME)
    if(name MATCHES "^(data|NOMPERS|VALMON|VERBES|OBJETS|NOMON|MESSAGE.*|.*[.]IMA|TERA[.].*)$")
        message(FATAL_ERROR "Forbidden runtime data in deployment: ${path}")
    endif()
endforeach()
file(COPY "${TERA_SDL_EXE}" "${TERA_SDL_DLL}" DESTINATION "${TERA_SMOKE_DIR}")
get_filename_component(exe_name "${TERA_SDL_EXE}" NAME)
execute_process(COMMAND "${CMAKE_COMMAND}" -E env
    "SDL_VIDEODRIVER=dummy" "SDL_RENDER_DRIVER=software" "SDL_AUDIODRIVER=dummy"
    "PATH=$ENV{SystemRoot}/System32" --unset=PYTHONHOME --unset=PYTHONPATH
    "${TERA_SMOKE_DIR}/${exe_name}" --smoke --save smoke.sav --capture smoke.bmp
    WORKING_DIRECTORY "${TERA_SMOKE_DIR}/other-cwd"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
file(WRITE "${TERA_SMOKE_DIR}/evidence.txt" "cwd=other-cwd; PATH=System32; no --data; Python vars unset\nexit=${result}\n${output}${error}")
if(NOT result EQUAL 0)
    message(FATAL_ERROR "SDL embedded runtime failed (${result}): ${output}${error}")
endif()
message("${output}")

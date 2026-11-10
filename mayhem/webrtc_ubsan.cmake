# Fuzz-build only (-DCMAKE_PROJECT_INCLUDE): the vendored WebRTC fixed-point DSP under src/common_audio
# wraps signed integers on purpose, so drop only that UBSan check there. Everything else is unchanged.
function(mayhem_relax_webrtc)
    file(GLOB_RECURSE webrtc_srcs "${CMAKE_SOURCE_DIR}/src/common_audio/*.c")
    set_property(SOURCE ${webrtc_srcs} TARGET_DIRECTORY pocketsphinx
                 APPEND PROPERTY COMPILE_OPTIONS -fno-sanitize=signed-integer-overflow)
endfunction()
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}" CALL mayhem_relax_webrtc)

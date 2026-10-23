cmake_minimum_required(VERSION 3.20)
project(ps5_quake3e_psvr2 C CXX ASM)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/param.json param_json)
string(JSON PS5_TITLE_NAME GET "${param_json}" localizedParameters en-US titleName)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS sce_sys/param.json)

set(QUAKE3E_DIR "${ROOT}/vendor/Quake3e" CACHE PATH "Quake3e checkout")
set(PS5_HUI_DIR "$ENV{HOME}/dev/ps5-homebrew-ui" CACHE PATH "ps5-homebrew-ui checkout")
set(PS5_OPENGL_PREFIX "$ENV{HOME}/dev/ps5-opengl/build/sdk/ps5-opengl-gl46" CACHE PATH "ps5-opengl SDK")
set(PS5_PAYLOAD_SDK "${PS5_HUI_DIR}/.deps/native/ps5-payload-sdk" CACHE PATH "PS5 Payload SDK")
set(Q ${QUAKE3E_DIR}/code)

foreach(required
  "${Q}/qcommon/common.c"
  "${PS5_HUI_DIR}/tooling/native/ps5-pie.ld"
  "${PS5_OPENGL_PREFIX}/manifest.sha256"
  "${PS5_PAYLOAD_SDK}/bin/prospero-lld")
  if(NOT EXISTS "${required}")
    message(FATAL_ERROR "missing required PSVR2 build input: ${required}")
  endif()
endforeach()

# Validate the OpenGL package and expose its headers. Final static linking is
# performed by tools/link-title-opengl.sh because a PS5 title still needs the
# native PIE -> eboot conversion step.
find_package(PS5OpenGL CONFIG REQUIRED
  PATHS "${PS5_OPENGL_PREFIX}/lib/cmake/PS5OpenGL"
  NO_DEFAULT_PATH)

file(GLOB client_sources ${Q}/client/*.c)
list(FILTER client_sources EXCLUDE REGEX "cl_curl\\.c$|snd_codec_ogg\\.c$")
file(GLOB qcommon_sources ${Q}/qcommon/*.c)
list(FILTER qcommon_sources EXCLUDE REGEX "vm_(x86|aarch64|armv7l|powerpc)\\.c$")
file(GLOB server_sources ${Q}/server/*.c)
list(FILTER server_sources EXCLUDE REGEX "sv_rankings\\.c$")
file(GLOB botlib_sources ${Q}/botlib/*.c)
file(GLOB renderer_sources ${Q}/renderercommon/*.c ${Q}/renderer/*.c)
file(GLOB jpeg_sources ${Q}/libjpeg/*.c)
set(asm_sources ${Q}/asm/snd_mix_x86_64.s)

set(engine_sources ${client_sources} ${qcommon_sources} ${server_sources} ${botlib_sources}
    ${renderer_sources} ${jpeg_sources})
set_source_files_properties(${engine_sources} PROPERTIES COMPILE_OPTIONS "-w")
set_source_files_properties(${botlib_sources} PROPERTIES COMPILE_DEFINITIONS BOTLIB)
set_source_files_properties(${Q}/qcommon/net_ip.c PROPERTIES COMPILE_DEFINITIONS
  "if_nametoindex=ps5q3_if_nametoindex;getnameinfo=ps5q3_getnameinfo;gai_strerror=ps5q3_gai_strerror;ioctl=ps5q3_ioctl;getaddrinfo=ps5q3_getaddrinfo;freeaddrinfo=ps5q3_freeaddrinfo")
set_source_files_properties(${Q}/qcommon/common.c PROPERTIES COMPILE_DEFINITIONS umask=ps5q3_umask)
set_source_files_properties(${asm_sources} PROPERTIES
  LANGUAGE ASM COMPILE_DEFINITIONS ELF COMPILE_OPTIONS "-x;assembler-with-cpp")

add_library(quake3e OBJECT
  ${engine_sources}
  ${asm_sources}
  q3/ps5_sys.c
  q3/ps5_shared.c
  q3/ps5_glimp.c
  q3/ps5_input.c
  q3/ps5_snd.c
  src/platform.c
  q3vr/ps5_q3vr.c
  q3vr/ps5_q3vr_gl.c
  q3vr/ps5_q3vr_engine.c
  q3vr/runtime/app_heap.c
  q3vr/runtime/ps5_opengl_shims.c)

target_compile_definitions(quake3e PRIVATE
  "PS5_TITLE_NAME=\"${PS5_TITLE_NAME}\""
  USE_OPENGL_API
  PS5_Q3VR
  NO_VM_COMPILED
  NDEBUG
  GL_GLEXT_PROTOTYPES=1)

target_include_directories(quake3e PRIVATE
  ${Q}
  ${Q}/libjpeg
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/q3vr
  ${PS5_OPENGL_PREFIX}/include)

# Let the imported target contribute any compile usage requirements as well.
target_link_libraries(quake3e PRIVATE PS5OpenGL::OpenGL)
set_target_properties(quake3e PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
target_compile_options(quake3e PRIVATE
  -O2 -g1 -ffunction-sections -fdata-sections -fno-omit-frame-pointer
  -fno-strict-aliasing -fno-fast-math)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/eboot.elf
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tools/link-title-opengl.sh
          ${CMAKE_BINARY_DIR} ${PS5_HUI_DIR} ${PS5_OPENGL_PREFIX} ${PS5_PAYLOAD_SDK}
          "$<TARGET_OBJECTS:quake3e>"
  DEPENDS quake3e "$<TARGET_OBJECTS:quake3e>"
          ${CMAKE_CURRENT_SOURCE_DIR}/tools/link-title-opengl.sh
  COMMAND_EXPAND_LISTS
  VERBATIM)
add_custom_target(title ALL DEPENDS ${CMAKE_BINARY_DIR}/eboot.elf)

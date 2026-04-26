
if(WIN32)
  set(PCR_PROC_SOURCES child_proc_win.cpp)
else()
  set(PCR_PROC_SOURCES child_proc_posix.cpp)
endif()


add_library(pcr_proc ${PCR_PROC_SOURCES})
add_library(pcr::proc ALIAS pcr_proc)

target_include_directories(pcr_proc PUBLIC ${PCR_ROOT_DIR})

target_compile_features(pcr_proc PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(pcr_proc PRIVATE /W4 /permissive-)
else()
  target_compile_options(pcr_proc PRIVATE -Wall -Wextra -Wpedantic)
endif()



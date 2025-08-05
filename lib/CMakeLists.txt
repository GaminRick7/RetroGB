file(GLOB sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/lib/*.cpp")
file(GLOB headers CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.hpp")

# Remove cpu_exec.cpp from sources since it's now split into separate files
list(REMOVE_ITEM sources "${PROJECT_SOURCE_DIR}/lib/cpu_exec.cpp")

add_library(emu STATIC ${sources} ${headers}
        cpu_registers.cpp
        cpu_stack.cpp
        cpu_interrupts.cpp
        cpu_instructions.cpp
        ppu_memory.cpp
        ppu_fifo.cpp
        ppu_sprites.cpp
        ppu_pipeline.cpp)

target_include_directories(emu
        PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

if (WIN32)
  target_include_directories(emu
          PUBLIC
          "${PROJECT_SOURCE_DIR}/../windows_deps/sdl2/include"
          "${PROJECT_SOURCE_DIR}/../windows_deps/sdl2_ttf/include"
  )
  target_link_libraries(emu
          PUBLIC
          ${SDL2_LIBRARIES}
          ${SDL2_TTF_LIBRARIES}
  )
else()
  target_include_directories(emu
          PUBLIC
          ${SDL2_INCLUDE_DIRS}
          ${SDL2_TTF_INCLUDE_DIRS}
  )
  target_link_libraries(emu
          PUBLIC
          ${SDL2_LIBRARIES}
          ${SDL2_TTF_LIBRARIES}
  )
  
  # Link macOS frameworks required by SDL2
  if(APPLE)
    target_link_libraries(emu
            PUBLIC
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
            "-framework Carbon"
            "-framework CoreAudio"
            "-framework AudioToolbox"
            "-framework ForceFeedback"
            "-framework AVFoundation"
            "-framework GameController"
            "-framework Metal"
            "-framework QuartzCore"
            "-framework CoreGraphics"
            "-framework ApplicationServices"
            "-framework Security"
            "-framework SystemConfiguration"
            "-framework CoreHaptics"
    )
  endif()
endif()

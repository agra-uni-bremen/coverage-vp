file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(rv32
		iss.cpp
		syscall.cpp
		coverage.cpp
		json.cpp
		basic_block.cpp
		addr2line.cpp
        ${HEADERS})

target_link_libraries(rv32 symex core-common ${SoftFloat_LIBRARIES}
	nlohmann_json::nlohmann_json dw)

if(COLOR_THEME STREQUAL "LIGHT")
	message("> using color theme LIGHT")
	target_compile_definitions(rv32 PRIVATE COLOR_THEME_LIGHT)
elseif(COLOR_THEME STREQUAL "DARK")
	message("> using color theme DARK")
	target_compile_definitions(rv32 PRIVATE COLOR_THEME_DARK)
endif()

target_include_directories(rv32 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

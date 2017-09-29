cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	if(DEFINED ENV{VITASDK})
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
	else()
		message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
	endif()
endif()

set(SHORT_NAME fontInstaller)
project(${SHORT_NAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Vita Font Installer")
set(VITA_TITLEID  "CXZV00001")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(${SHORT_NAME}
	main.c
	utils.c
)

target_link_libraries(${SHORT_NAME}
	vita2d
	freetype
	vita-parser
	quirc
	m
	png
	jpeg
	z
	ScePvf_stub
	ScePgf_stub
	SceGxm_stub
	SceDisplay_stub
	SceLibKernel_stub
	SceIofilemgr_stub 
	SceSysmodule_stub 
	SceCommonDialog_stub 
	SceCtrl_stub
	ScePower_stub
	SceAppUtil_stub
	taihen_stub
)

vita_create_self(eboot.bin ${SHORT_NAME} UNSAFE)
vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} eboot.bin
	VERSION ${VITA_VERSION}
	NAME ${VITA_APP_NAME}
	FILE sce_sys/icon0.png sce_sys/icon0.png
		 sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
		 sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
		 sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
		 module/build/fontRedirect.suprx module/fontInstaller.suprx
)

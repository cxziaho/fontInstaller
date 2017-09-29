#include "utils.h"

#include <psp2/io/fcntl.h> 
#include <psp2/apputil.h>
#include <psp2/system_param.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define TAI_CONFIG_PATH "ux0:tai/config.txt"
#define TAI_CONFIG_RECOVERY_PATH "ur0:tai/config.txt"
#define BACKUP_CONFIG_PATH "ux0:data/font/backup.txt"
#define PLUGIN_PATH "ux0:app/CXZV00001/module/fontInstaller.suprx"
#define PLUGIN_CONFIG_PATH "ux0:data/font/config.txt"

int exists(const char *path) {
	int fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
	if (fd < 0)
		return 0;
	sceIoClose(fd);
	return 1;
}

// Code taken from VitaShell
int allocateReadFile(const char *file, char **buffer) {
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;

	int size = sceIoLseek32(fd, 0, SCE_SEEK_END);
	sceIoLseek32(fd, 0, SCE_SEEK_SET);

	*buffer = malloc(size);
	if (!*buffer) {
		sceIoClose(fd);
		return -1;
	}

	int read = sceIoRead(fd, *buffer, size);
	sceIoClose(fd);

	return read;
}
//

int isPluginInstalled() {
	char *pluginsFolder;
	if (exists(TAI_CONFIG_PATH))
		allocateReadFile(TAI_CONFIG_PATH, &pluginsFolder);
	else
		allocateReadFile(TAI_CONFIG_RECOVERY_PATH, &pluginsFolder);
	if (strstr(pluginsFolder, PLUGIN_PATH) != NULL)
		return 1;
	return 0;
}

int installPlugin() {
	char *plugins;
	int length;
	if (exists(TAI_CONFIG_PATH)) {
		length = allocateReadFile(TAI_CONFIG_PATH, &plugins);
	} else {
		length = allocateReadFile(TAI_CONFIG_RECOVERY_PATH, &plugins);
	}
	if (!exists(BACKUP_CONFIG_PATH)) {
		int fd = sceIoOpen(BACKUP_CONFIG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
		if (fd < 0)
			return fd;
		int ret = sceIoWrite(fd, plugins, strlen(plugins));
		sceIoClose(fd);
	}
	char *ptr = strstr(strstr(plugins, "*main\n"), "ur0:tai/henkaku.suprx\n");
	int pos = (ptr - plugins) + strlen("ur0:tai/henkaku.suprx\n");
	memmove(plugins + pos + strlen(PLUGIN_PATH "\n"), plugins + pos, strlen(plugins) + strlen(PLUGIN_PATH "\n"));
	memcpy(plugins + pos, PLUGIN_PATH "\n", strlen(PLUGIN_PATH "\n"));
	if (exists(TAI_CONFIG_PATH)) {
		int fd = sceIoOpen(TAI_CONFIG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
		if (fd < 0)
			return fd;
		int ret = sceIoWrite(fd, plugins, strlen(plugins));
		sceIoClose(fd);
	} else {
		int fd = sceIoOpen(TAI_CONFIG_RECOVERY_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
		if (fd < 0)
			return fd;
		int ret = sceIoWrite(fd, plugins, strlen(plugins));
		sceIoClose(fd);
	}
	return 0;
}

int setFont(const char *font) {
	sceIoRemove(PLUGIN_CONFIG_PATH);
	int fd = sceIoOpen(PLUGIN_CONFIG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return fd;
	int ret = sceIoWrite(fd, font, strlen(font));
	sceIoClose(fd);
	return ret;
}

char *getCurrentFont() {
	char font_name[32];
	SceUID fd = sceIoOpen(PLUGIN_CONFIG_PATH, SCE_O_RDONLY, 0);
	if (fd < 0)
		return "none";
	int read = sceIoRead(fd, &font_name, 32);
	for (int i = read-1; i >= 0; i--)
		if (font_name[i] == '\n' || font_name[i] == ' ') { font_name[i] = 0; break; }
	return strdup(font_name);
}

// Code adapted from VitaShell
int getEnterButton() {
	SceAppUtilInitParam init_param;
	SceAppUtilBootParam boot_param;
	memset(&init_param, 0, sizeof(SceAppUtilInitParam));
	memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&init_param, &boot_param);

	int enterButton;
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &enterButton);
	
	if (enterButton == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE)
		return 1;
	else
		return 0;
}
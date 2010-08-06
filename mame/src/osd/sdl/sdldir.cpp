//============================================================
//
//  sdldir.c - SDL core directory access functions
//
//  Copyright (c) 1996-2010, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//  SDLMAME by Olivier Galibert and R. Belmont
//
//============================================================

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#ifdef SDLMAME_LINUX
#define __USE_LARGEFILE64
#endif
#ifndef SDLMAME_BSD
#ifdef _XOPEN_SOURCE
#undef _XOPEN_SOURCE
#endif
#define _XOPEN_SOURCE 500
#endif

//#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef __USE_BSD
#define __USE_BSD	// to get DT_xxx on Linux
#endif
#undef _POSIX_C_SOURCE  // to get DT_xxx on OS X
#include <dirent.h>

#include "osdcore.h"
#include "sdlos.h"

#if defined(SDLMAME_WIN32) || defined(SDLMAME_OS2)
#define PATHSEPCH '\\'
#define INVPATHSEPCH '/'
#else
#define PATHSEPCH '/'
#define INVPATHSEPCH '\\'
#endif

#if defined(SDLMAME_DARWIN) || defined(SDLMAME_WIN32) || defined(SDLMAME_NO64BITIO) || defined(SDLMAME_BSD) || defined(SDLMAME_OS2)
typedef struct dirent sdl_dirent;
typedef struct stat sdl_stat;
#define sdl_readdir readdir
#define sdl_stat_fn stat
#else
typedef struct dirent64 sdl_dirent;
typedef struct stat64 sdl_stat;
#define sdl_readdir readdir64
#define sdl_stat_fn stat64
#endif

#define HAS_DT_XXX defined(SDLMAME_LINUX) || defined(SDLMAME_BSD) || defined(SDLMAME_DARWIN)

#ifdef _MSC_VER
// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <tchar.h>

struct _osd_directory
{
	HANDLE				find;					// handle to the finder
	int					is_first;				// TRUE if this is the first entry
	osd_directory_entry	entry;					// current entry's data
	WIN32_FIND_DATA		data;					// current raw data
};
#else
struct _osd_directory
{
	osd_directory_entry ent;
	sdl_dirent *data;
	DIR *fd;
	char *path;
};
#endif

static char *build_full_path(const char *path, const char *file)
{
	char *ret = (char *) osd_malloc(strlen(path)+strlen(file)+2);
	char *p = ret;

	strcpy(p, path);
	p += strlen(path);
	*p++ = PATHSEPCH;
	strcpy(p, file);
	return ret;
}


#if HAS_DT_XXX
static osd_dir_entry_type get_attributes_enttype(int attributes)
{
	if (attributes == DT_DIR)
		return ENTTYPE_DIR;
	else
		return ENTTYPE_FILE;
}
#else

static osd_dir_entry_type get_attributes_stat(const char *file)
{
	sdl_stat st;
	if(sdl_stat_fn(file, &st))
		return (osd_dir_entry_type) 0;

#ifdef _MSC_VER
	if (S_IFDIR & st.st_mode)
#else
	if (S_ISDIR(st.st_mode))
#endif
		return ENTTYPE_DIR;
	else
		return ENTTYPE_FILE;
}
#endif

static UINT64 osd_get_file_size(const char *file)
{
	sdl_stat st;
	if(sdl_stat_fn(file, &st))
		return 0;
	return st.st_size;
}

//============================================================
//  osd_opendir
//============================================================

#ifdef _MSC_VER
#define tstring_from_utf8	astring_from_utf8
#define utf8_from_tstring	utf8_from_astring

//============================================================
//  astring_from_utf8
//============================================================

CHAR *astring_from_utf8(const char *utf8string)
{
	WCHAR *wstring;
	int char_count;
	CHAR *result;

	// convert MAME string (UTF-8) to UTF-16
	char_count = MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, NULL, 0);
	wstring = (WCHAR *)osd_malloc(char_count * sizeof(*wstring));
	MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, wstring, char_count);

	// convert UTF-16 to "ANSI code page" string
	char_count = WideCharToMultiByte(CP_ACP, 0, wstring, -1, NULL, 0, NULL, NULL);
	result = (CHAR *)osd_malloc(char_count * sizeof(*result));
	if (result != NULL)
		WideCharToMultiByte(CP_ACP, 0, wstring, -1, result, char_count, NULL, NULL);

    osd_free(wstring);

	return result;
}


//============================================================
//  utf8_from_astring
//============================================================

char *utf8_from_astring(const CHAR *astring)
{
	WCHAR *wstring;
	int char_count;
	CHAR *result;

	// convert "ANSI code page" string to UTF-16
	char_count = MultiByteToWideChar(CP_ACP, 0, astring, -1, NULL, 0);
	wstring = (WCHAR *)osd_malloc(char_count * sizeof(*wstring));
	MultiByteToWideChar(CP_ACP, 0, astring, -1, wstring, char_count);

	// convert UTF-16 to MAME string (UTF-8)
	char_count = WideCharToMultiByte(CP_UTF8, 0, wstring, -1, NULL, 0, NULL, NULL);
	result = (CHAR *)osd_malloc(char_count * sizeof(*result));
	if (result != NULL)
		WideCharToMultiByte(CP_UTF8, 0, wstring, -1, result, char_count, NULL, NULL);

    osd_free(wstring);

	return result;
}


//============================================================
//  wstring_from_utf8
//============================================================

WCHAR *wstring_from_utf8(const char *utf8string)
{
	int char_count;
	WCHAR *result;

	// convert MAME string (UTF-8) to UTF-16
	char_count = MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, NULL, 0);
	result = (WCHAR *)osd_malloc(char_count * sizeof(*result));
	if (result != NULL)
		MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, result, char_count);

	return result;
}


//============================================================
//  utf8_from_wstring
//============================================================

char *utf8_from_wstring(const WCHAR *wstring)
{
	int char_count;
	char *result;

	// convert UTF-16 to MAME string (UTF-8)
	char_count = WideCharToMultiByte(CP_UTF8, 0, wstring, -1, NULL, 0, NULL, NULL);
	result = (char *)osd_malloc(char_count * sizeof(*result));
	if (result != NULL)
		WideCharToMultiByte(CP_UTF8, 0, wstring, -1, result, char_count, NULL, NULL);

	return result;
}

#endif

osd_directory *osd_opendir(const char *dirname)
{
#ifdef _MSC_VER
	osd_directory *dir = NULL;
	TCHAR *t_dirname = NULL;
	TCHAR *dirfilter = NULL;
	size_t dirfilter_size;

	// allocate memory to hold the osd_tool_directory structure
	dir = (osd_directory *)malloc(sizeof(*dir));
	if (dir == NULL)
		goto error;
	memset(dir, 0, sizeof(*dir));

	// initialize the structure
	dir->find = INVALID_HANDLE_VALUE;
	dir->is_first = TRUE;

	// convert the path to TCHARs
	t_dirname = tstring_from_utf8(dirname);
	if (t_dirname == NULL)
		goto error;

	// append \*.* to the directory name
	dirfilter_size = _tcslen(t_dirname) + 5;
	dirfilter = (TCHAR *)malloc(dirfilter_size * sizeof(*dirfilter));
	if (dirfilter == NULL)
		goto error;
	_sntprintf(dirfilter, dirfilter_size, TEXT("%s\\*.*"), t_dirname);

	// attempt to find the first file
	dir->find = FindFirstFile(dirfilter, &dir->data);

error:
	// cleanup
	if (t_dirname != NULL)
		osd_free(t_dirname);
	if (dirfilter != NULL)
		free(dirfilter);
	if (dir != NULL && dir->find == INVALID_HANDLE_VALUE)
	{
		free(dir);
		dir = NULL;
	}
	return dir;
#else
	osd_directory *dir = NULL;
	char *tmpstr, *envstr;
	int i, j;

	dir = (osd_directory *) osd_malloc(sizeof(osd_directory));
	if (dir)
	{
		memset(dir, 0, sizeof(osd_directory));
		dir->fd = NULL;
	}

	tmpstr = (char *) osd_malloc(strlen(dirname)+1);
	strcpy(tmpstr, dirname);

	if (tmpstr[0] == '$')
	{
		char *envval;
		envstr = (char *) osd_malloc(strlen(tmpstr)+1);

		strcpy(envstr, tmpstr);

		i = 0;
		while (envstr[i] != PATHSEPCH && envstr[i] != INVPATHSEPCH && envstr[i] != 0 && envstr[i] != '.')
		{
			i++;
		}

		envstr[i] = '\0';

		envval = osd_getenv(&envstr[1]);
		if (envval != NULL)
		{
			j = strlen(envval) + strlen(tmpstr) + 1;
			osd_free(tmpstr);
			tmpstr = (char *) osd_malloc(j);

			// start with the value of $HOME
			strcpy(tmpstr, envval);
			// replace the null with a path separator again
			envstr[i] = PATHSEPCH;
			// append it
			strcat(tmpstr, &envstr[i]);
		}
		else
			fprintf(stderr, "Warning: osd_opendir environment variable %s not found.\n", envstr);
		osd_free(envstr);
	}

	dir->fd = opendir(tmpstr);
	dir->path = tmpstr;

	if (dir && (dir->fd == NULL))
	{
		osd_free(dir->path);
		osd_free(dir);
		dir = NULL;
	}

	return dir;
#endif
}


//============================================================
//  osd_readdir
//============================================================

#ifdef _MSC_VER
//============================================================
//  win_attributes_to_entry_type
//============================================================

osd_dir_entry_type win_attributes_to_entry_type(DWORD attributes)
{
	if (attributes == 0xFFFFFFFF)
		return ENTTYPE_NONE;
	else if (attributes & FILE_ATTRIBUTE_DIRECTORY)
		return ENTTYPE_DIR;
	else
		return ENTTYPE_FILE;
}
#endif

const osd_directory_entry *osd_readdir(osd_directory *dir)
{
#ifdef _MSC_VER
	// if we've previously allocated a name, free it now
	if (dir->entry.name != NULL)
	{
		osd_free((void *)dir->entry.name);
		dir->entry.name = NULL;
	}

	// if this isn't the first file, do a find next
	if (!dir->is_first)
	{
		if (!FindNextFile(dir->find, &dir->data))
			return NULL;
	}

	// otherwise, just use the data we already had
	else
		dir->is_first = FALSE;

	// extract the data
	dir->entry.name = utf8_from_tstring(dir->data.cFileName);
	dir->entry.type = win_attributes_to_entry_type(dir->data.dwFileAttributes);
	dir->entry.size = dir->data.nFileSizeLow | ((UINT64) dir->data.nFileSizeHigh << 32);
	return (dir->entry.name != NULL) ? &dir->entry : NULL;
#else
	char *temp;
	dir->data = sdl_readdir(dir->fd);

	if (dir->data == NULL)
		return NULL;

	dir->ent.name = dir->data->d_name;
	temp = build_full_path(dir->path, dir->data->d_name);
	#if HAS_DT_XXX
	dir->ent.type = get_attributes_enttype(dir->data->d_type);
	#else
	dir->ent.type = get_attributes_stat(temp);
	#endif
	dir->ent.size = osd_get_file_size(temp);
	osd_free(temp);
	return &dir->ent;
#endif
}


//============================================================
//  osd_closedir
//============================================================

void osd_closedir(osd_directory *dir)
{
#ifdef _MSC_VER
	// free any data associated
	if (dir->entry.name != NULL)
		osd_free((void *)dir->entry.name);
	if (dir->find != INVALID_HANDLE_VALUE)
		FindClose(dir->find);
	free(dir);
#else
	if (dir->fd != NULL)
		closedir(dir->fd);
	osd_free(dir->path);
	osd_free(dir);
#endif
}


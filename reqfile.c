#include <kos.h>
#include <kos/fs.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>


#define MAXFILES 100
#define RQPAGESIZE 18


static struct fileentries {
	char filename[256];
	char path[256];
	int flags;
} thefiles[MAXFILES];

static int maxfiles;
static int rootpos;

static maple_device_t *cont;

/****************************************************************************
 * get_buttons
 *
 ****************************************************************************/

static unsigned int get_buttons()
{
	cont_state_t *cstate = (cont_state_t *)maple_dev_status(cont);
	return cstate->buttons;
}

/****************************************************************************
 * ParseDirectory
 *
 * Parse the directory, returning the number of files found
 ****************************************************************************/

int parse_dir (char *path)
{
	DIR *dir;
	DIR *test_dir;
	struct dirent *dirent = 0;
	char file_name[512];
	FILE *file;
	int i;

    // for dir parent
    strncpy(thefiles[0].path, path, 256);
    thefiles[0].path[255] = 0;

	maxfiles = 0;
	/* open directory */
	if ((dir = opendir(path)) == 0)
		return 0;

	while ((dirent = readdir(dir)) != 0)
	{
		if (dirent->d_name[0] == '.') continue;
		/* get stats */
		sprintf( file_name, "%s/%s", path, dirent->d_name );
		/* check directory (d_type == DT_DIR) */
		if (dirent->d_type == 4)
		{
			/* test it */
			if ((test_dir = opendir(file_name)) == 0) continue;
			closedir(test_dir);
			memset(&thefiles[maxfiles], 0, sizeof (struct fileentries));
			strncpy(thefiles[maxfiles].path, path, 256);
			thefiles[maxfiles].path[255] = 0;
			strncpy(thefiles[maxfiles].filename, dirent->d_name, 256);
			thefiles[maxfiles].filename[255] = 0;
			thefiles[maxfiles].flags = 1;
			maxfiles++;
		}
		else
		{
			/* test it */
			if ((file = fopen(file_name, "rb")) == 0) continue;
			fclose(file);
			memset(&thefiles[maxfiles], 0, sizeof (struct fileentries));
			strncpy(thefiles[maxfiles].path, path, 256);
			thefiles[maxfiles].path[255] = 0;
			strncpy(thefiles[maxfiles].filename, dirent->d_name, 256);
			thefiles[maxfiles].filename[255] = 0;
			maxfiles++;
		}

		if (maxfiles == MAXFILES)
			break;
	}
	/* close dir */
	closedir(dir);

    if (maxfiles < 2)
        return maxfiles; // early out - no need for sorting

	// sort them!
	for (i=0; i<maxfiles-1; i++)
	{
		char tempfilename[256];
		char temppath[256];
		int tempflags;

		if ((!thefiles[i].flags && thefiles[i+1].flags) || // directories first
			(thefiles[i].flags && thefiles[i+1].flags && strcasecmp(thefiles[i].filename, thefiles[i+1].filename) > 0) ||
			(!thefiles[i].flags && !thefiles[i+1].flags && strcasecmp(thefiles[i].filename, thefiles[i+1].filename) > 0))
		{
			strcpy(tempfilename, thefiles[i].filename);
			strcpy(temppath, thefiles[i].path);
			tempflags = thefiles[i].flags;
			strcpy(thefiles[i].filename, thefiles[i+1].filename);
			strcpy(thefiles[i].path, thefiles[i+1].path);
			thefiles[i].flags = thefiles[i+1].flags;
			strcpy(thefiles[i+1].filename, tempfilename);
			strcpy(thefiles[i+1].path, temppath);
			thefiles[i+1].flags = tempflags;
			i = -1;
		}
	}

	return maxfiles;
}

/****************************************************************************
 * ShowFiles
 *
 * Support function for FileSelector
 ****************************************************************************/

extern void gui_PrePrint(void);
extern void gui_PostPrint(void);
extern int gui_PrintWidth(char *text);
extern void gui_Print(char *text, uint32 fc, uint32 bc, int x, int y);

void ShowFiles( int offset, int selection )
{
	int i,j;
	char text[60];

	gui_PrePrint();

    if (!maxfiles)
    {
        strcpy(text, "EMPTY - Press Y to go back a level");
		gui_Print(text, 0xFFAAAAAA, 0xFF000000, 320 - gui_PrintWidth(text)/2, 24);
        gui_PostPrint();
        return;
    }

	j = 0;
	for ( i = offset; i < ( offset + RQPAGESIZE ) && i < maxfiles ; i++ )
	{
		if ( thefiles[i].flags )
		{
			strcpy(text,"[");
			strncat(text, thefiles[i].filename,51);
			strcat(text,"]");
		}
		else
			strncpy(text, thefiles[i].filename, 53);
		text[54]=0;

		gui_Print(text, j == (selection-offset) ? 0xFFFFFFFF : 0xFFAAAAAA, 0xFF000000, 320 - gui_PrintWidth(text)/2, (i - offset + 1)*24);

		j++;
	}

	gui_PostPrint();
}

/****************************************************************************
 * FileSelector
 *
 * Press X to select, O to cancel, and Triangle to go back a level
 ****************************************************************************/

int FileSelector()
{
	int offset = 0;
	int selection = 0;
	int havefile = 0;
	int redraw = 1;
	unsigned int p = get_buttons();

	while ( havefile == 0 && !(p & CONT_B) )
	{
		if ( redraw )
			ShowFiles( offset, selection );
		redraw = 0;

		while (!(p = get_buttons()))
			thd_sleep(10);
		while (p == get_buttons())
			thd_sleep(10);

		if ( p & CONT_DPAD_DOWN )
		{
			selection++;
			if ( selection >= maxfiles )
				selection = offset = 0;	// wrap around to top

			if ( ( selection - offset ) == RQPAGESIZE )
				offset += RQPAGESIZE; // next "page" of entries

			redraw = 1;
		}

		if ( p & CONT_DPAD_UP )
		{
			selection--;
			if ( selection < 0 )
			{
				selection = maxfiles - 1;
				offset = maxfiles > RQPAGESIZE ? selection - RQPAGESIZE + 1 : 0; // wrap around to bottom
			}

			if ( selection < offset )
			{
				offset -= RQPAGESIZE; // previous "page" of entries
				if ( offset < 0 )
					offset = 0;
			}

			redraw = 1;
		}

		if ( p & CONT_DPAD_RIGHT )
		{
			selection += RQPAGESIZE;
			if ( selection >= maxfiles )
				selection = offset = 0;	// wrap around to top

			if ( ( selection - offset ) >= RQPAGESIZE )
				offset += RQPAGESIZE; // next "page" of entries

			redraw = 1;
		}

		if ( p & CONT_DPAD_LEFT )
		{
			selection -= RQPAGESIZE;
			if ( selection < 0 )
			{
				selection = maxfiles - 1;
				offset = maxfiles > RQPAGESIZE ? selection - RQPAGESIZE + 1 : 0; // wrap around to bottom
			}

			if ( selection < offset )
			{
				offset -= RQPAGESIZE; // previous "page" of entries
				if ( offset < 0 )
					offset = 0;
			}

			redraw = 1;
		}

		if ( p & CONT_A )
		{
			if ( thefiles[selection].flags )	/*** This is directory ***/
			{
				char fname[256];

				strncpy(fname, thefiles[selection].path, 256);
				fname[255] = 0;
				strcat(fname, "/");
				strncat(fname, thefiles[selection].filename, 256 - strlen(fname));
				fname[255] = 0;
				//strcat(fname, "/");
				offset = selection = 0;
				parse_dir(fname);
			}
			else
				return selection;

			redraw = 1;
		}

		if ( p & CONT_Y )
		{
			char fname[256];
			int pathpos = strlen(thefiles[0].path);

			while (pathpos > rootpos)
			{
				pathpos--;
				if (thefiles[0].path[pathpos+1] == '/')
					break;
			}
			if (pathpos < rootpos) pathpos = rootpos; /* handle root case */
			strncpy(fname, thefiles[0].path, pathpos+1);
			fname[pathpos+1] = 0;
			offset = selection = 0;
			parse_dir(fname);

			redraw = 1;
		}
	}

	return -1; // no file selected
}

/****************************************************************************
 * RequestFile
 *
 * return pointer to filename selected
 ****************************************************************************/

char *RequestFile (char *initialPath)
{
	int selection, i;
	static char fname[512];

	cont = NULL;
	while (cont == NULL)
		cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

	for (i=1; i<strlen(initialPath); i++)
		if (initialPath[i] == '/')
			break;
	rootpos = i - 1;

	parse_dir (initialPath);
	selection = FileSelector ();
	if (selection < 0)
		return 0;

	strncpy (fname, thefiles[selection].path, 256);
	fname[255] = 0;
	strcat(fname, "/");
	strncat (fname, thefiles[selection].filename, 256);
	fname[511] = 0;

	return fname;
}

/*	Public domain	*/
/*
 * This program demonstrates the use of the file loader widget,
 * AG_FileDlg.
 */

#include <agar/core.h>
#include <agar/gui.h>

#include <string.h>
#include <unistd.h>

static void
LoadBMP(AG_Event *event)
{
	char *file = AG_STRING(1);
	SDL_Surface *bmp;
	AG_Window *win;

	/* Load the bitmap file into a SDL surface. */
	if ((bmp = SDL_LoadBMP(file)) == NULL) {
		AG_TextMsg(AG_MSG_ERROR, "%s: %s", file, SDL_GetError());
		return;
	}

	win = AG_WindowNew(0);
	AG_WindowSetCaption(win, "Image <%s>", file);

	/*
	 * Add a pixmap widget displaying the bitmap, scaled down to the
	 * maximum view size if needed.
	 */
	AG_PixmapFromSurfaceScaled(win, 0, bmp,
	    (bmp->w < (agView->w - 16)) ? bmp->w : agView->w - 16,
	    (bmp->h < (agView->h - 40)) ? bmp->h : agView->h - 40);

	SDL_FreeSurface(bmp);
	AG_WindowShow(win);
}

static void
CreateWindow(void)
{
	AG_Window *win;
	AG_FileDlg *fd;

	win = AG_WindowNew(0);

	/* Create the file loader widget. */
	fd = AG_FileDlgNew(win, AG_FILEDLG_EXPAND);
	
	/* Set a default filename. */
	AG_FileDlgSetFilename(fd, "sample.bmp");

	/* Register the loader functions. */
	AG_FileDlgAddType(fd, "Bitmap file", "*.bmp", LoadBMP, NULL);
	
	AG_WindowShow(win);
}

int
main(int argc, char *argv[])
{
	if (AG_InitCore("loader-demo", 0) == -1) {
		fprintf(stderr, "%s\n", AG_GetError());
		return (1);
	}
	if (AG_InitVideo(640, 480, 32, AG_VIDEO_RESIZABLE) == -1) {
		fprintf(stderr, "%s\n", AG_GetError());
		return (-1);
	}
	AG_BindGlobalKey(SDLK_ESCAPE, KMOD_NONE, AG_Quit);
	AG_BindGlobalKey(SDLK_F8, KMOD_NONE, AG_ViewCapture);
	CreateWindow();
	AG_EventLoop();
	AG_Destroy();
	return (0);
}


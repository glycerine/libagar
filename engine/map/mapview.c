/*	$Csoft: mapview.c,v 1.22 2005/06/21 08:09:07 vedge Exp $	*/

/*
 * Copyright (c) 2002, 2003, 2004, 2005 CubeSoft Communications, Inc.
 * <http://www.csoft.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <engine/engine.h>

#ifdef MAP

#include <engine/view.h>
#include <engine/input.h>

#include <engine/widget/widget.h>
#include <engine/widget/window.h>
#include <engine/widget/primitive.h>
#include <engine/widget/button.h>
#include <engine/widget/label.h>
#include <engine/widget/tlist.h>
#include <engine/widget/combo.h>
#include <engine/widget/textbox.h>

#include "map.h"
#include "mapview.h"
#include "select.h"

#include <stdarg.h>
#include <string.h>

const struct widget_ops mapview_ops = {
	{
		NULL,		/* init */
		NULL,		/* reinit */
		mapview_destroy,
		NULL,		/* load */
		NULL,		/* save */
		NULL		/* edit */
	},
	mapview_draw,
	mapview_scale
};

enum {
	ZOOM_MIN =	 20,	/* Min zoom factor (%) */
	ZOOM_MAX =	 500,	/* Max zoom factor (%) */
	ZOOM_PROPS_MIN = 60,	/* Min zoom factor for showing properties (%) */
	ZOOM_GRID_MIN =	 20	/* Min zoom factor for showing the grid (%) */
};

int	mapview_bg = 1;			/* Background tiles enable */
int	mapview_bg_moving = 1;		/* Background tiles moving */
int	mapview_bg_sqsize = 8;		/* Background tile size */
int	mapview_sel_bounded = 0;	/* Restrict edition to selection */

static void lost_focus(int, union evarg *);
static void mousemotion(int, union evarg *);
static void mousebuttondown(int, union evarg *);
static void mousebuttonup(int, union evarg *);
static void key_up(int, union evarg *);
static void key_down(int, union evarg *);

struct mapview *
mapview_new(void *parent, struct map *m, int flags, struct toolbar *toolbar,
    struct statusbar *statbar)
{
	struct mapview *mv;

	mv = Malloc(sizeof(struct mapview), M_OBJECT);
	mapview_init(mv, m, flags, toolbar, statbar);
	object_attach(parent, mv);
	return (mv);
}

#ifdef EDITION

void
mapview_select_tool(struct mapview *mv, struct tool *ntool, void *p)
{
	struct window *pwin;

	if (mv->curtool != NULL) {
		if (mv->curtool->trigger != NULL) {
			widget_set_bool(mv->curtool->trigger, "state", 0);
		}
		if (mv->curtool->win != NULL) {
			window_hide(mv->curtool->win);
		}
		mv->curtool->mv = NULL;

		widget_replace_surface(mv->status, mv->status->surface,
		    text_render(NULL, -1, COLOR(TEXT_COLOR),
		    _("Select a tool.")));
	}
	mv->curtool = ntool;

	if (ntool != NULL) {
		ntool->p = p;
		ntool->mv = mv;

		if (ntool->trigger != NULL) {
			widget_set_bool(ntool->trigger, "state", 1);
		}
		if (ntool->win != NULL) {
			window_show(ntool->win);
		}
		tool_update_status(ntool);
	}

	if ((pwin = widget_parent_window(mv)) != NULL) {
		view->focus_win = pwin;
		widget_focus(mv);
	}
}

static void
sel_tool(int argc, union evarg *argv)
{
	struct mapview *mv = argv[1].p;
	struct tool *tool = argv[2].p;
	void *p = argv[3].p;

	if (mv->curtool == tool) {
		mapview_select_tool(mv, NULL, NULL);
	} else {
		mapview_select_tool(mv, tool, p);
	}
}

#if 0
static void
update_tooltips(int argc, union evarg *argv)
{
	struct mapview *mv = argv[1].p;
	struct tool *tool = argv[2].p;
	int in = argv[3].i;

	if (in) {
		mapview_status(mv, "%s", _(tool->desc));
	} else {
		tool_update_status(tool);
	}
}
#endif

struct tool *
mapview_reg_tool(struct mapview *mv, const struct tool *tool, void *p)
{
	struct tool *ntool;

	ntool = Malloc(sizeof(struct tool), M_MAPEDIT);
	memcpy(ntool, tool, sizeof(struct tool));
	ntool->p = p;
	tool_init(ntool, mv);

	if (((tool->flags & TOOL_HIDDEN) == 0) && mv->toolbar != NULL) {
		SDL_Surface *icon = ntool->icon >= 0 ? ICON(ntool->icon) : NULL;

		ntool->trigger = toolbar_add_button(mv->toolbar,
		    mv->toolbar->nrows-1, icon, 1, 0, sel_tool,
		    "%p, %p, %p", mv, ntool, p);
#if 0
		event_new(ntool->trigger, "button-mouseoverlap",
		    update_tooltips, "%p, %p", mv, ntool);
#endif
	}

	TAILQ_INSERT_TAIL(&mv->tools, ntool, tools);
	return (ntool);
}

void
mapview_set_default_tool(struct mapview *mv, struct tool *tool)
{
	mv->deftool = tool;
}

void
mapview_reg_draw_cb(struct mapview *mv,
    void (*draw_func)(struct mapview *, void *), void *p)
{
	struct mapview_draw_cb *dcb;

	dcb = Malloc(sizeof(struct mapview_draw_cb), M_WIDGET);
	dcb->func = draw_func;
	dcb->p = p;
	SLIST_INSERT_HEAD(&mv->draw_cbs, dcb, draw_cbs);
}

void
mapview_selected_layer(int argc, union evarg *argv)
{
	struct combo *com = argv[0].p;
	struct mapview *mv = argv[1].p;
	struct tlist_item *it = argv[2].p;
	struct tlist *tl = com->list;
	int i = 0;

	TAILQ_FOREACH(it, &tl->items, items) {
		if (it->selected) {
			struct map_layer *lay = it->p1;

			mv->map->cur_layer = i;
			textbox_printf(com->tbox, "%d. %s", i, lay->name);
			return;
		}
		i++;
	}
	text_msg(MSG_ERROR, _("No layer is selected."));
}
#endif /* EDITION */

void
mapview_destroy(void *p)
{
	struct mapview *mv = p;
	struct mapview_draw_cb *dcb, *ndcb;

	for (dcb = SLIST_FIRST(&mv->draw_cbs);
	     dcb != SLIST_END(&mv->draw_cbs);
	     dcb = ndcb) {
		ndcb = SLIST_NEXT(dcb, draw_cbs);
		Free(dcb, M_WIDGET);
	}
	widget_destroy(mv);
}

static void
mapview_detached(int argc, union evarg *argv)
{
	struct mapview *mv = argv[0].p;
	struct tool *tool, *ntool;

	for (tool = TAILQ_FIRST(&mv->tools);
	     tool != TAILQ_END(&mv->tools);
	     tool = ntool) {
		ntool = TAILQ_NEXT(tool, tools);
		tool_destroy(tool);
		Free(tool, M_MAPEDIT);
	}
	TAILQ_INIT(&mv->tools);
	mv->curtool = NULL;
}

static void
dblclick_expired(int argc, union evarg *argv)
{
	struct mapview *mv = argv[0].p;

	mv->dblclicked = 0;
}

static void
scrolled_bar(int argc, union evarg *argv)
{
	struct scrollbar *sb = argv[0].p;
	struct mapview *mv = argv[1].p;

	mapview_update_camera(mv);
}

static void
map_resized(int argc, union evarg *argv)
{
	struct mapview *mv = argv[0].p;

	mapview_update_camera(mv);
}

void
mapview_init(struct mapview *mv, struct map *m, int flags,
    struct toolbar *toolbar, struct statusbar *statbar)
{
	widget_init(mv, "mapview", &mapview_ops,
	    WIDGET_FOCUSABLE|WIDGET_CLIPPING|WIDGET_WFILL|WIDGET_HFILL);
	object_wire_gfx(mv, "/engine/map/pixmaps/pixmaps");

	mv->flags = (flags | MAPVIEW_CENTER);
	mv->map = m;
	mv->cam = 0;
	mv->mw = 0;					/* Set on scale */
	mv->mh = 0;
	mv->prew = 4;
	mv->preh = 4;
	mv->prop_style = 0;
	mv->mouse.scrolling = 0;
	mv->mouse.x = 0;
	mv->mouse.y = 0;
	mv->mouse.xmap = 0;
	mv->mouse.ymap = 0;
	mv->dblclicked = 0;
	mv->toolbar = toolbar;
	mv->statusbar = statbar;
	mv->status = (statbar != NULL) ?
	             statusbar_add_label(statbar, LABEL_STATIC, "...") : NULL;
	mv->art_tl = NULL;
	mv->objs_tl = NULL;
	mv->layers_tl = NULL;
	mv->curtool = NULL;
	mv->deftool = NULL;
	TAILQ_INIT(&mv->tools);
	SLIST_INIT(&mv->draw_cbs);
	
	mv->cx = -1;
	mv->cy = -1;
	mv->cxrel = 0;
	mv->cyrel = 0;
	mv->cxoffs = 0;
	mv->cyoffs = 0;
	mv->xoffs = 0;
	mv->yoffs = 0;
	
	mv->msel.set = 0;
	mv->msel.x = 0;
	mv->msel.y = 0;
	mv->msel.xoffs = 0;
	mv->msel.yoffs = 0;
	mv->esel.set = 0;
	mv->esel.moving = 0;
	mv->esel.x = 0;
	mv->esel.y = 0;
	mv->esel.w = 0;
	mv->esel.h = 0;
	mv->rsel.moving = 0;
	
	pthread_mutex_lock(&m->lock);
	mv->mx = m->origin.x;
	mv->my = m->origin.y;
	pthread_mutex_unlock(&m->lock);

	event_new(mv, "widget-lostfocus", lost_focus, NULL);
	event_new(mv, "widget-hidden", lost_focus, NULL);
	event_new(mv, "window-keyup", key_up, NULL);
	event_new(mv, "window-keydown", key_down, NULL);
	event_new(mv, "window-mousemotion", mousemotion, NULL);
	event_new(mv, "window-mousebuttondown", mousebuttondown, NULL);
	event_new(mv, "window-mousebuttonup", mousebuttonup, NULL);
	event_new(mv, "detached", mapview_detached, NULL);
	event_new(mv, "dblclick-expire", dblclick_expired, NULL);
	event_new(mv, "map-resized", map_resized, NULL);
}

void
mapview_set_scrollbars(struct mapview *mv, struct scrollbar *hbar,
    struct scrollbar *vbar)
{
	mv->hbar = hbar;
	mv->vbar = vbar;

	if (hbar != NULL) {
		WIDGET(hbar)->flags &= ~(WIDGET_FOCUSABLE);
		WIDGET(hbar)->flags |= WIDGET_UNFOCUSED_MOTION;
		widget_bind(mv->hbar, "value", WIDGET_INT, &MV_CAM(mv).x);
		event_new(mv->hbar, "scrollbar-changed", scrolled_bar, "%p",
		    mv);
	}
	if (vbar != NULL) {
		WIDGET(vbar)->flags &= ~(WIDGET_FOCUSABLE);
		WIDGET(vbar)->flags |= WIDGET_UNFOCUSED_MOTION;
		widget_bind(mv->vbar, "value", WIDGET_INT, &MV_CAM(mv).y);
		event_new(mv->vbar, "scrollbar-changed", scrolled_bar, "%p",
		    mv);
	}
}

/*
 * Translate widget coordinates to node coordinates.
 * The map must be locked.
 */
static __inline__ void
get_node_coords(struct mapview *mv, int *x, int *y)
{
	*x -= mv->xoffs;
	*y -= mv->yoffs;
	mv->cxoffs = *x % MV_TILESZ(mv);
	mv->cyoffs = *y % MV_TILESZ(mv);

	*x = ((*x) - mv->cxoffs)/MV_TILESZ(mv);
	*y = ((*y) - mv->cyoffs)/MV_TILESZ(mv);

	mv->cx = mv->mx + *x;
	mv->cy = mv->my + *y;

	if (mv->cx < 0 || mv->cx >= mv->map->mapw || mv->cxoffs < 0)
		mv->cx = -1;
	if (mv->cy < 0 || mv->cy >= mv->map->maph || mv->cyoffs < 0)
		mv->cy = -1;
}

/* Draw the node property icons. */
void
mapview_draw_props(struct mapview *mv, struct node *node, int x, int y,
    int mx, int my)
{
	const struct {
		Uint32	flag;
		Uint32	sprite;
	} flags[] = {
		{ NODEREF_WALK,		MAPVIEW_WALK },
		{ NODEREF_CLIMB,	MAPVIEW_CLIMB },
		{ NODEREF_BIO,		MAPVIEW_BIO },
		{ NODEREF_REGEN,	MAPVIEW_REGEN }
	};
	const struct {
		Uint32	edge;
		Uint32	sprite;
	} edges[] = {
		{ NODEREF_EDGE_E,	MAPVIEW_EDGE_E },
		{ NODEREF_EDGE_N,	MAPVIEW_EDGE_N },
		{ NODEREF_EDGE_S,	MAPVIEW_EDGE_S },
		{ NODEREF_EDGE_W,	MAPVIEW_EDGE_W },
		{ NODEREF_EDGE_NW,	MAPVIEW_EDGE_NW },
		{ NODEREF_EDGE_NE,	MAPVIEW_EDGE_NE },
		{ NODEREF_EDGE_SW,	MAPVIEW_EDGE_SW },
		{ NODEREF_EDGE_SE,	MAPVIEW_EDGE_SE }
	};
	const int nflags = sizeof(flags) / sizeof(flags[0]);
	const int nedges = sizeof(edges) / sizeof(edges[0]);
	struct noderef *r;
	int i;

	if (mv->prop_style > 0)
		widget_blit(mv, ICON(mv->prop_style), x, y);

	if (mx == mv->map->origin.x && my == mv->map->origin.y) {
		widget_blit(mv, SPRITE(mv,MAPVIEW_ORIGIN).su, x, y);
		x += (SPRITE(mv,MAPVIEW_ORIGIN).su)->w;
	}

	TAILQ_FOREACH(r, &node->nrefs, nrefs) {
		if (r->layer != mv->map->cur_layer) {
			continue;
		}
		for (i = 0; i < nflags; i++) {
			if ((r->flags & flags[i].flag) == 0) {
				continue;
			}
			widget_blit(mv, SPRITE(mv,flags[i].sprite).su, x, y);
			x += (SPRITE(mv,flags[i].sprite).su)->w;
		}
		for (i = 0; i < nedges; i++) {
			if (r->r_gfx.edge != edges[i].edge) {
				continue;
			}
			widget_blit(mv, SPRITE(mv,edges[i].sprite).su, x, y);
			x += (SPRITE(mv,edges[i].sprite).su)->w;
		}
	}
}

static void
draw_cursor(struct mapview *mv)
{
	SDL_Rect rd;
	int msx, msy;

	rd.w = MV_TILESZ(mv);
	rd.h = MV_TILESZ(mv);

	if (mv->msel.set) {
		mouse_get_state(&msx, &msy);
		rd.x = msx;
		rd.y = msy;
		/* XXX opengl */
		if (!view->opengl) {
			SDL_BlitSurface(ICON(SELECT_CURSORBMP), NULL,
			    view->v, &rd);
		}
		return;
	}

	rd.x = mv->mouse.x*MV_TILESZ(mv) + mv->xoffs;
	rd.y = mv->mouse.y*MV_TILESZ(mv) + mv->yoffs;

	if (mv->curtool == NULL)
		return;

	if (mv->curtool->cursor_su != NULL) {
		rd.x += WIDGET(mv)->cx;
		rd.y += WIDGET(mv)->cy;
		/* XXX opengl */
		if (!view->opengl) {
			SDL_BlitSurface(mv->curtool->cursor_su, NULL,
			    view->v, &rd);
		}
	} else {
		if (mv->curtool->cursor != NULL) {
			if (mv->curtool->cursor(mv->curtool, &rd) == -1)
				goto defcurs;
		}
	}
	return;
defcurs:
	primitives.rect_outlined(mv,
	    rd.x + 1,
	    rd.y + 1,
	    MV_TILESZ(mv) - 1,
	    MV_TILESZ(mv) - 1,
	    COLOR(MAPVIEW_CURSOR_COLOR));
	primitives.rect_outlined(mv,
	    rd.x + 2,
	    rd.y + 2,
	    MV_TILESZ(mv) - 3,
	    MV_TILESZ(mv) - 3,
	    COLOR(MAPVIEW_CURSOR_COLOR));
}

static __inline__ void
center_to_origin(struct mapview *mv)
{
	MV_CAM(mv).x = mv->map->origin.x*MV_TILESZ(mv);
	MV_CAM(mv).y = mv->map->origin.y*MV_TILESZ(mv);
	mapview_update_camera(mv);
}

void
mapview_draw(void *p)
{
#ifdef EDITION
	extern int mapedition;
#endif
	struct mapview *mv = p;
	struct mapview_draw_cb *dcb;
	struct map *m = mv->map;
	struct node *node;
	struct noderef *nref;
	int mx, my, rx, ry;
	int layer = 0;
	int esel_x = -1, esel_y = -1, esel_w = -1, esel_h = -1;
	int msel_x = -1, msel_y = -1, msel_w = -1, msel_h = -1;
	SDL_Rect rExtent;
#ifdef HAVE_OPENGL
	GLboolean blend_save;
	GLenum blend_sfactor;
	GLenum blend_dfactor;
	GLfloat texenvmode;
#endif

	if (WIDGET(mv)->w < TILESZ || WIDGET(mv)->h < TILESZ)
		return;

	SLIST_FOREACH(dcb, &mv->draw_cbs, draw_cbs)
		dcb->func(mv, dcb->p);
	
	if (mv->flags & MAPVIEW_CENTER) {
		mv->flags &= ~(MAPVIEW_CENTER);
		center_to_origin(mv);
	}

	if (mapview_bg &&
	    (mv->flags & MAPVIEW_NO_BG) == 0) {
		SDL_Rect rtiling;

		rtiling.x = 0;
		rtiling.y = 0;
		rtiling.w = WIDGET(mv)->w;
		rtiling.h = WIDGET(mv)->h;
		primitives.tiling(mv, rtiling, mapview_bg_sqsize, 0,
		    COLOR(MAPVIEW_TILE1_COLOR),
		    COLOR(MAPVIEW_TILE2_COLOR));
	}

#ifdef HAVE_OPENGL
	if (view->opengl) {
		glGetBooleanv(GL_BLEND, &blend_save);
		glGetIntegerv(GL_BLEND_SRC, &blend_sfactor);
		glGetIntegerv(GL_BLEND_DST, &blend_dfactor);
		glGetTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &texenvmode);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
#endif
	pthread_mutex_lock(&m->lock);
	if (m->map == NULL)
		goto out;
draw_layer:
	if (!m->layers[layer].visible) {
		goto next_layer;
	}
	for (my = mv->my, ry = mv->yoffs;
	     ((my - mv->my) <= mv->mh) && (my < m->maph);
	     my++, ry += MV_TILESZ(mv)) {

		for (mx = mv->mx, rx = mv->xoffs;
	     	     ((mx - mv->mx) <= mv->mw) && (mx < m->mapw);
		     mx++, rx += MV_TILESZ(mv)) {

			node = &m->map[my][mx];

			TAILQ_FOREACH(nref, &node->nrefs, nrefs) {
				if (nref->layer != layer)
					continue;

				noderef_draw(m, nref,
				    WIDGET(mv)->cx + rx,
				    WIDGET(mv)->cy + ry,
				    mv->cam);

				if ((nref->flags & NODEREF_SELECTED) &&
				    noderef_extent(m, nref, &rExtent, mv->cam)
				    == 0) {
					primitives.rect_outlined(mv,
					    rx + rExtent.x - 1,
					    ry + rExtent.y - 1,
					    rExtent.w + 1,
					    rExtent.h + 1,
					    COLOR(MAPVIEW_RSEL_COLOR));
				}
			}

			if ((mv->flags & MAPVIEW_PROPS) &&
			    MV_ZOOM(mv) >= ZOOM_PROPS_MIN) {
				mapview_draw_props(mv, node, rx, ry, mx, my);
			}
			if ((mv->flags & MAPVIEW_GRID) &&
			    MV_ZOOM(mv) >= ZOOM_GRID_MIN) {
				/* XXX overdraw */
				primitives.rect_outlined(mv,
				    rx, ry,
				    MV_TILESZ(mv) + 1,
				    MV_TILESZ(mv) + 1,
				    COLOR(MAPVIEW_GRID_COLOR));
			}
#ifdef EDITION
			if (!mapedition)
				continue;

			/* Indicate the mouse/effective selections. */
			if (mv->msel.set &&
			    mv->msel.x == mx && mv->msel.y == my) {
				msel_x = rx + 1;
				msel_y = ry + 1;
				msel_w = mv->msel.xoffs*MV_TILESZ(mv) - 2;
				msel_h = mv->msel.yoffs*MV_TILESZ(mv) - 2;
			}
			
			if (mv->esel.set &&
			    mv->esel.x == mx && mv->esel.y == my) {
				esel_x = rx;
				esel_y = ry;
				esel_w = MV_TILESZ(mv)*mv->esel.w;
				esel_h = MV_TILESZ(mv)*mv->esel.h;
			}
#endif /* EDITION */
		}
	}
next_layer:
	if (++layer < m->nlayers)
		goto draw_layer;			/* Draw next layer */

#ifdef EDITION
	/* Indicate the selection. */
	if (esel_x != -1) {
		primitives.rect_outlined(mv,
		    esel_x, esel_y,
		    esel_w, esel_h,
		    COLOR(MAPVIEW_ESEL_COLOR));
	}
	if (msel_x != -1) {
		primitives.rect_outlined(mv,
		    msel_x, msel_y,
		    msel_w, msel_h,
		    COLOR(MAPVIEW_MSEL_COLOR));
	}

	/* Draw the cursor for the current tool. */
	if ((mv->flags & MAPVIEW_EDIT) &&
	    (mv->flags & MAPVIEW_NO_CURSOR) == 0 &&
	    (mv->cx != -1 && mv->cy != -1)) {
		draw_cursor(mv);
	}
#endif /* EDITION */

out:
#ifdef HAVE_OPENGL
	if (view->opengl) {
		if (blend_save) {
			glEnable(GL_BLEND);
		} else {
			glDisable(GL_BLEND);
		}
		glBlendFunc(blend_sfactor, blend_dfactor);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, texenvmode);
	}
#endif
	pthread_mutex_unlock(&m->lock);
}

/*
 * Recalculate the offsets to be used by the rendering routine based on
 * the current camera coordinates. 
 */
void
mapview_update_camera(struct mapview *mv)
{
	struct map_camera *cam;
	int xcam, ycam;

	pthread_mutex_lock(&mv->map->lock);
	cam = &MV_CAM(mv);
	
	if (cam->x < 0) {
		cam->x = 0;
	} else if (cam->x >= mv->map->mapw*MV_TILESZ(mv)) { 
		cam->x = mv->map->mapw*MV_TILESZ(mv) - 1;
	}
	if (cam->y < 0) {
		cam->y = 0;
	} else if (cam->y >= mv->map->maph*MV_TILESZ(mv)) { 
		cam->y = mv->map->maph*MV_TILESZ(mv) - 1;
	}

	xcam = cam->x;
	ycam = cam->y;

	switch (cam->alignment) {
	case MAP_CENTER:
		xcam -= WIDGET(mv)->w/2;
		ycam -= WIDGET(mv)->h/2;
		break;
	case MAP_LOWER_CENTER:
		xcam -= WIDGET(mv)->w/2;
		ycam -= WIDGET(mv)->h;
		break;
	case MAP_UPPER_CENTER:
		xcam -= WIDGET(mv)->w/2;
		break;
	case MAP_UPPER_LEFT:
		break;
	case MAP_MIDDLE_LEFT:
		ycam -= WIDGET(mv)->h/2;
		break;
	case MAP_LOWER_LEFT:
		ycam -= WIDGET(mv)->h;
		break;
	case MAP_UPPER_RIGHT:
		xcam -= WIDGET(mv)->w;
		break;
	case MAP_MIDDLE_RIGHT:
		xcam -= WIDGET(mv)->w;
		ycam -= WIDGET(mv)->h/2;
		break;
	case MAP_LOWER_RIGHT:
		xcam -= WIDGET(mv)->w;
		ycam -= WIDGET(mv)->h;
		break;
	}

	mv->mx = xcam / MV_TILESZ(mv);
	if (mv->mx < 0) {
		mv->mx = 0;
		mv->xoffs = -xcam;
	} else {
		mv->xoffs = -(xcam % MV_TILESZ(mv));
	}
	
	mv->my = ycam / MV_TILESZ(mv);
	if (mv->my < 0) {
		mv->my = 0;
		mv->yoffs = -ycam;
	} else {
		mv->yoffs = -(ycam % MV_TILESZ(mv));
	}
	pthread_mutex_unlock(&mv->map->lock);

	if (mv->hbar != NULL) {
		widget_set_int(mv->hbar, "min", 0);
		widget_set_int(mv->hbar, "max", mv->map->mapw*MV_TILESZ(mv));
		scrollbar_set_bar_size(mv->hbar, 20);
	}
	if (mv->vbar != NULL) {
		widget_set_int(mv->vbar, "min", 0);
		widget_set_int(mv->vbar, "max", mv->map->maph*MV_TILESZ(mv));
		scrollbar_set_bar_size(mv->vbar, 20);
	}
}

void
mapview_set_scale(struct mapview *mv, u_int zoom, int adj_offs)
{
	extern int magnifier_zoom_toval;
	int old_tilesz = MV_TILESZ(mv);
	int x, y;
	int old_pixw = mv->map->mapw*MV_TILESZ(mv);
	int old_pixh = mv->map->maph*MV_TILESZ(mv);
	int pixw, pixh;

	if (zoom < ZOOM_MIN) { zoom = ZOOM_MIN; }
	else if (zoom > ZOOM_MAX) { zoom = ZOOM_MAX; }
	
	magnifier_zoom_toval = zoom;

	MV_ZOOM(mv) = zoom;
	MV_TILESZ(mv) = zoom*TILESZ/100;

	if (MV_TILESZ(mv) > MAP_MAX_TILESZ)
		MV_TILESZ(mv) = MAP_MAX_TILESZ;

	mv->mw = WIDGET(mv)->w/MV_TILESZ(mv) + 1;
	mv->mh = WIDGET(mv)->h/MV_TILESZ(mv) + 1;

	SDL_GetMouseState(&x, &y);
	x -= WIDGET(mv)->cx;
	y -= WIDGET(mv)->cy;

	pixw = mv->map->mapw*MV_TILESZ(mv);
	pixh = mv->map->maph*MV_TILESZ(mv);

	if (adj_offs) {
		MV_CAM(mv).x = MV_CAM(mv).x * pixw / old_pixw;
		MV_CAM(mv).y = MV_CAM(mv).y * pixh / old_pixh;
	}
	mapview_update_camera(mv);
}

static __inline__ int
inside_nodesel(struct mapview *mv, int x, int y)
{
	return (!mapview_sel_bounded || !mv->esel.set ||
	    (x >= mv->esel.x &&
	     y >= mv->esel.y &&
	     x <  mv->esel.x + mv->esel.w &&
	     y <  mv->esel.y + mv->esel.h));
}

static void
mousemotion(int argc, union evarg *argv)
{
	struct mapview *mv = argv[0].p;
	int x = argv[1].i;
	int y = argv[2].i;
	int xrel = argv[3].i;
	int yrel = argv[4].i;
	int state = argv[5].i;

	pthread_mutex_lock(&mv->map->lock);
	get_node_coords(mv, &x, &y);
	mv->cxrel = x - mv->mouse.x;
	mv->cyrel = y - mv->mouse.y;
	mv->mouse.xmap = x*MV_TILESZ(mv) + mv->xoffs;
	mv->mouse.ymap = y*MV_TILESZ(mv) + mv->yoffs;

	if ((mv->flags & MAPVIEW_EDIT) && mv->curtool != NULL) {
		if (mv->curtool->effect != NULL &&
		    state & SDL_BUTTON(1) &&
		    mv->cx != -1 && mv->cy != -1 &&
		    (x != mv->mouse.x || y != mv->mouse.y) &&
		    (inside_nodesel(mv, mv->cx, mv->cy))) {
			if (mv->curtool->effect(mv->curtool,
			    &mv->map->map[mv->cy][mv->cx]) == 1)
				goto out;
		}
		if (mv->curtool->mousemotion != NULL &&
		    mv->curtool->mousemotion(mv->curtool,
		      mv->mouse.xmap, mv->mouse.ymap,
		      xrel, yrel, state) == 1) {
			goto out;
		}
		if (mv->deftool != NULL &&
		    mv->deftool->mousemotion != NULL &&
		    mv->deftool->mousemotion(mv->deftool,
		      mv->mouse.xmap, mv->mouse.ymap,
		      xrel, yrel, state) == 1) {
			goto out;
		}
	}
	
	if (mv->mouse.scrolling) {
		MV_CAM(mv).x -= xrel;
		MV_CAM(mv).y -= yrel;
		mapview_update_camera(mv);
	} else if (mv->msel.set) {
		mv->msel.xoffs += mv->cxrel;
		mv->msel.yoffs += mv->cyrel;
	} else if (mv->esel.set && mv->esel.moving) {
		select_update_nodemove(mv, mv->cxrel, mv->cyrel);
	} else if (mv->rsel.moving) {
		select_update_refmove(mv, xrel, yrel);
	}
out:
	mv->mouse.x = x;
	mv->mouse.y = y;
	pthread_mutex_unlock(&mv->map->lock);
}

static void
mousebuttondown(int argc, union evarg *argv)
{
	extern int magnifier_zoom_inc;
	struct mapview *mv = argv[0].p;
	struct map *m = mv->map;
	int button = argv[1].i;
	int x = argv[2].i;
	int y = argv[3].i;
	struct tool *tool;
	
	widget_focus(mv);
	
	pthread_mutex_lock(&m->lock);
	get_node_coords(mv, &x, &y);
	mv->mouse.x = x;
	mv->mouse.y = y;
	mv->mouse.xmap = mv->cx*MV_TILESZ(mv) + mv->cxoffs;
	mv->mouse.ymap = mv->cy*MV_TILESZ(mv) + mv->cyoffs;

	if ((mv->flags & MAPVIEW_EDIT) && (mv->cx >= 0 && mv->cy >= 0)) {
		if (mv->curtool != NULL) {
			if (button == SDL_BUTTON_LEFT &&
			    mv->curtool->effect != NULL &&
			    inside_nodesel(mv, mv->cx, mv->cy)) {
				if (mv->curtool->effect(mv->curtool,
				    &m->map[mv->cy][mv->cx]) == 1) {
					goto out;
				}
			}
			if (mv->curtool->mousebuttondown != NULL &&
			    mv->curtool->mousebuttondown(mv->curtool,
			      mv->mouse.xmap, mv->mouse.ymap, button) == 1) {
				goto out;
			}
		}

		/* Mouse bindings allow inactive tools to bind mouse events. */
		TAILQ_FOREACH(tool, &mv->tools, tools) {
			struct tool_mbinding *mbinding;

			SLIST_FOREACH(mbinding, &tool->mbindings, mbindings) {
				if (mbinding->button != button) {
					continue;
				}
				if (mbinding->edit &&
				   (OBJECT(m)->flags & OBJECT_READONLY)) {
					continue;
				}
				tool->mv = mv;
				if (mbinding->func(tool, button, 1,
				    mv->mouse.xmap, mv->mouse.ymap,
				    mbinding->arg) == 1)
					goto out;
			}
		}

		if (mv->deftool != NULL &&
		    mv->deftool->mousebuttondown != NULL &&
		    mv->deftool->mousebuttondown(mv->deftool,
		      mv->mouse.xmap, mv->mouse.ymap, button) == 1) {
			goto out;
		}
	}

	switch (button) {
	case SDL_BUTTON_LEFT:
		if (mv->esel.set) {
			if (mv->cx >= mv->esel.x &&
			    mv->cy >= mv->esel.y &&
			    mv->cx < mv->esel.x+mv->esel.w &&
			    mv->cy < mv->esel.y+mv->esel.h) {
				select_begin_nodemove(mv);
			} else {
				mv->esel.set = 0;
			}
			goto out;
		} else {
			struct noderef *r;
			int nx, ny;
			
			if ((SDL_GetModState() & KMOD_CTRL) == 0) {
				for (ny = 0; ny < m->maph; ny++) {
					for (nx = 0; nx < m->mapw; nx++) {
						struct node *node =
						    &m->map[ny][nx];
						
						TAILQ_FOREACH(r, &node->nrefs,
						    nrefs) {
							r->flags &=
							    ~(NODEREF_SELECTED);
						}
					}
				}
			}
			if ((r = noderef_locate(m, mv->mouse.xmap,
			    mv->mouse.ymap, mv->cam)) != NULL) {
				if (r->flags & NODEREF_SELECTED) {
					r->flags &= ~(NODEREF_SELECTED);
				} else {
					r->flags |= NODEREF_SELECTED;
					mv->rsel.moving = 1;
				}
			}
			if (SDL_GetModState() & KMOD_SHIFT) {
				if ((mv->flags & MAPVIEW_NO_NODESEL) == 0) {
					select_begin_nodesel(mv);
					goto out;
				}
			}
		}
		if (mv->dblclicked) {
			event_cancel(mv, "dblclick-expire");
			event_post(NULL, mv, "mapview-dblclick",
			    "%i, %i, %i, %i, %i", button, x, y,
			    mv->cxoffs, mv->cyoffs);
			mv->dblclicked = 0;
		} else {
			mv->dblclicked++;
			event_schedule(NULL, mv, mouse_dblclick_delay,
			    "dblclick-expire", NULL);
		}
		break;
	case SDL_BUTTON_MIDDLE:
		/* TODO menu */
		if ((mv->flags & MAPVIEW_EDIT) == 0 ||
		    mv->curtool == NULL) {
		    	mv->mouse.scrolling++;
			break;
		}
		break;
	case SDL_BUTTON_RIGHT:
		mv->mouse.scrolling++;
		goto out;
	case SDL_BUTTON_WHEELDOWN:
		if ((mv->flags & MAPVIEW_NO_BMPZOOM) == 0) {
			mapview_set_scale(mv, MV_ZOOM(mv)-magnifier_zoom_inc,
			    1);
			mapview_status(mv, _("%d%% zoom"), MV_ZOOM(mv));
		}
		break;
	case SDL_BUTTON_WHEELUP:
		if ((mv->flags & MAPVIEW_NO_BMPZOOM) == 0) {
			mapview_set_scale(mv, MV_ZOOM(mv)+magnifier_zoom_inc,
			    1);
			mapview_status(mv, _("%d%% zoom"), MV_ZOOM(mv));
		}
		break;
	}
out:
	pthread_mutex_unlock(&m->lock);
}

static void
mousebuttonup(int argc, union evarg *argv)
{
	struct mapview *mv = argv[0].p;
	struct map *m = mv->map;
	int button = argv[1].i;
	int x = argv[2].i;
	int y = argv[3].i;
	struct tool *tool;
	
	pthread_mutex_lock(&m->lock);
	get_node_coords(mv, &x, &y);

	if ((mv->flags & MAPVIEW_EDIT) && (mv->cx >= 0 && mv->cy >= 0)) {
		if (mv->curtool != NULL) {
			if (mv->curtool->mousebuttonup != NULL &&
			    mv->curtool->mousebuttonup(mv->curtool,
			      mv->mouse.xmap, mv->mouse.ymap, button) == 1) {
				goto out;
			}
		}
		
		TAILQ_FOREACH(tool, &mv->tools, tools) {
			struct tool_mbinding *mbinding;

			SLIST_FOREACH(mbinding, &tool->mbindings, mbindings) {
				if (mbinding->button != button) {
					continue;
				}
				if (mbinding->edit &&
				    (OBJECT(m)->flags & OBJECT_READONLY)) {
					continue;
				}
				tool->mv = mv;
				if (mbinding->func(tool, button, 0, x, y,
				    mbinding->arg) == 1)
					goto out;
			}
		}
		
		if (mv->deftool != NULL &&
		    mv->deftool->mousebuttonup != NULL &&
		    mv->deftool->mousebuttonup(mv->deftool,
		      mv->mouse.xmap, mv->mouse.ymap, button) == 1) {
			goto out;
		}
	} else {
		mv->mouse.scrolling = 0;
		if (mv->esel.set && mv->esel.moving) {
			select_end_nodemove(mv);
		}
		mv->rsel.moving = 0;
		goto out;
	}

	switch (button) {
	case SDL_BUTTON_LEFT:
		if (mv->msel.set &&
		   (mv->msel.xoffs == 0 || mv->msel.yoffs == 0)) {
			mv->esel.set = 0;
			mv->msel.set = 0;
		} else {
			if (mv->msel.set) {
				select_end_nodesel(mv);
				mv->msel.set = 0;
			} else if (mv->esel.set && mv->esel.moving) {
				select_end_nodemove(mv);
			}
		}
		mv->rsel.moving = 0;
		break;
	case SDL_BUTTON_RIGHT:
	case SDL_BUTTON_MIDDLE:
		mv->mouse.scrolling = 0;
		break;
	}
out:
	pthread_mutex_unlock(&m->lock);
}

static void
key_up(int argc, union evarg *argv)
{
	struct mapview *mv = argv[0].p;
	int keysym = argv[1].i;
	int keymod = argv[2].i;
	struct tool *tool;
	
	pthread_mutex_lock(&mv->map->lock);
	
	if (mv->flags & MAPVIEW_EDIT &&
	    mv->curtool != NULL &&
	    mv->curtool->keyup != NULL &&
	    mv->curtool->keyup(mv->curtool, keysym, keymod) == 1)
		goto out;
	
	TAILQ_FOREACH(tool, &mv->tools, tools) {
		struct tool_kbinding *kbinding;

		SLIST_FOREACH(kbinding, &tool->kbindings, kbindings) {
			if (kbinding->key == keysym &&
			    (kbinding->mod == KMOD_NONE ||
			     keymod & kbinding->mod)) {
				if (kbinding->edit &&
				   (((mv->flags & MAPVIEW_EDIT) == 0) ||
				    ((OBJECT(mv->map)->flags &
				     OBJECT_READONLY)))) {
					continue;
				}
				tool->mv = mv;
				if (kbinding->func(tool, keysym, 0,
				    kbinding->arg) == 1)
					goto out;
			}
		}
	}
out:
	pthread_mutex_unlock(&mv->map->lock);
}

static void
key_down(int argc, union evarg *argv)
{
	struct mapview *mv = argv[0].p;
	int keysym = argv[1].i;
	int keymod = argv[2].i;
	struct tool *tool;
	
	pthread_mutex_lock(&mv->map->lock);
	
	if (mv->flags & MAPVIEW_EDIT &&
	    mv->curtool != NULL &&
	    mv->curtool->keydown != NULL &&
	    mv->curtool->keydown(mv->curtool, keysym, keymod) == 1)
		goto out;

	TAILQ_FOREACH(tool, &mv->tools, tools) {
		struct tool_kbinding *kbinding;

		SLIST_FOREACH(kbinding, &tool->kbindings, kbindings) {
			if (kbinding->key == keysym &&
			    (kbinding->mod == KMOD_NONE ||
			     keymod & kbinding->mod)) {
				if (kbinding->edit &&
				   (((mv->flags & MAPVIEW_EDIT) == 0) ||
				    ((OBJECT(mv->map)->flags &
				     OBJECT_READONLY)))) {
					continue;
				}
				tool->mv = mv;
				if (kbinding->func(tool, keysym, 1,
				    kbinding->arg) == 1)
					goto out;
			}
		}
	}

	switch (keysym) {
	case SDLK_o:
		center_to_origin(mv);
		break;
	case SDLK_p:
		if (keymod & KMOD_SHIFT) {
			if (++mv->prop_style == MAPVIEW_FRAMES_END) {
				mv->prop_style = 0;
			}
		} else {
			if (mv->flags & MAPVIEW_PROPS) {
				mv->flags &= ~(MAPVIEW_PROPS);
			} else {
				mv->flags |= MAPVIEW_PROPS;
			}
		}
		break;
	case SDLK_g:
		if (mv->flags & MAPVIEW_GRID) {
			mv->flags &= ~(MAPVIEW_GRID);
		} else {
			mv->flags |= MAPVIEW_GRID;
		}
		break;
	case SDLK_b:
		if (mv->flags & MAPVIEW_NO_BG) {
			mv->flags &= ~(MAPVIEW_NO_BG);
		} else {
			mv->flags |= MAPVIEW_NO_BG;
		}
		break;
	}
out:	
	pthread_mutex_unlock(&mv->map->lock);
}

void
mapview_scale(void *p, int rw, int rh)
{
	struct mapview *mv = p;

	if (rw == -1 && rh == -1) {
		WIDGET(mv)->w = mv->prew*TILESZ;
		WIDGET(mv)->h = mv->preh*TILESZ;
		if (mv->hbar != NULL) {
			WIDGET_OPS(mv->hbar)->scale(mv->hbar, -1, -1);
		}
		if (mv->vbar != NULL) {
			WIDGET_OPS(mv->vbar)->scale(mv->vbar, -1, -1);
		}
		return;
	}

	if (mv->hbar != NULL) {
		WIDGET(mv->hbar)->x = 0;
		WIDGET(mv->hbar)->y = WIDGET(mv)->h - mv->hbar->button_size;
		WIDGET(mv->hbar)->w = WIDGET(mv)->w;
		WIDGET(mv->hbar)->h = mv->hbar->button_size;
		widget_scale(mv->hbar, WIDGET(mv->hbar)->w,
		   WIDGET(mv->hbar)->h);
	}
	if (mv->vbar != NULL) {
		WIDGET(mv->vbar)->x = 0;
		WIDGET(mv->vbar)->y = 0;
		WIDGET(mv->vbar)->w = mv->vbar->button_size;
		WIDGET(mv->vbar)->h = WIDGET(mv)->h;
		widget_scale(mv->vbar, WIDGET(mv->vbar)->w,
		    WIDGET(mv->vbar)->h);
	}
	
	pthread_mutex_lock(&mv->map->lock);
	mapview_set_scale(mv, MV_ZOOM(mv), 0);
	pthread_mutex_unlock(&mv->map->lock);
}

static void
lost_focus(int argc, union evarg *argv)
{
	struct mapview *mv = argv[0].p;

	event_cancel(mv, "dblclick-expire");
}

/* Set the coordinates and geometry of the selection rectangle. */
void
mapview_set_selection(struct mapview *mv, int x, int y, int w, int h)
{
	mv->msel.set = 0;
	mv->esel.set = 1;
	mv->esel.x = x;
	mv->esel.y = y;
	mv->esel.w = w;
	mv->esel.h = h;
}

/* Fetch the coordinates and geometry of the selection rectangle. */
int
mapview_get_selection(struct mapview *mv, int *x, int *y, int *w, int *h)
{
	if (mv->esel.set) {
		if (x != NULL) { *x = mv->esel.x; }
		if (y != NULL) { *y = mv->esel.y; }
		if (w != NULL) { *w = mv->esel.w; }
		if (h != NULL) { *h = mv->esel.h; }
		return (1);
	} else {
		return (0);
	}
}

void	
mapview_prescale(struct mapview *mv, int w, int h)
{
	mv->prew = w;
	mv->preh = h;
}

void
mapview_status(struct mapview *mv, const char *fmt, ...)
{
	char status[LABEL_MAX];
	va_list ap;

	if (mv->status == NULL)
		return;

	va_start(ap, fmt);
	vsnprintf(status, sizeof(status), fmt, ap);
	va_end(ap);

	widget_replace_surface(mv->status, mv->status->surface,
	    text_render(NULL, -1, COLOR(TEXT_COLOR), status));
}

#endif /* MAP */

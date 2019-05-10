/*
 * vim:ts=4:sw=4:expandtab
 *
 * i3 - an improved dynamic tiling window manager
 * © 2009 Michael Stapelberg and contributors (see also: LICENSE)
 *
 * x.c: Interface to X11, transfers our in-memory state to X11 (see also
 *      render.c). Basically a big state machine.
 *
 */
#include "all.h"

/*
 * Apply shape to top corners when possible
 */
void rounded_top(Con *con, xcb_pixmap_t *pid, xcb_gcontext_t *black) {

    uint16_t w  = con->rect.width;
    uint16_t h  = con->rect.height;

    int32_t r = config.corners.size;
    int32_t d = r * 2;

    xcb_arc_t arcs[] = {
      { -1, 0, d, d, 0, 360 << 6 },
      { w-d, 0, d, d, 0, 360 << 6 },
    };

    xcb_poly_fill_arc(conn, *pid, *black, 2, arcs);
}

/*
 * Apply shape to bottom corners when possible
 */
void rounded_bottom(Con *con, xcb_pixmap_t *pid, xcb_gcontext_t *black) {

    uint16_t w  = con->rect.width;
    uint16_t h  = con->rect.height;

    int32_t r = config.corners.size;
    int32_t d = r * 2;

    xcb_arc_t arcs[] = {
       { -1, h-d, d, d, 0, 360 << 6 },
       { w-d, h-d, d, d, 0, 360 << 6 }
    };

    xcb_poly_fill_arc(conn, *pid, *black, 2, arcs);
}

/*
 * Apply shape to top corners when possible
 */
void triangled_top(Con *con, xcb_pixmap_t *pid, xcb_gcontext_t *black) {

    int32_t size = config.corners.size;
    uint16_t w  = con->rect.width;
    uint16_t h  = con->rect.height;


    xcb_point_t top_left_triangle[] = {
        {0, 0}, {size, 0}, {0, size}, {0, 0}
    };

    xcb_point_t top_right_triangle[] = {
        {w-size, 0}, {w, 0}, {w, size}, {w-size, 0}
    };

    xcb_fill_poly(conn, *pid, *black, XCB_POLY_SHAPE_CONVEX, XCB_COORD_MODE_ORIGIN, 4, top_left_triangle);
    xcb_fill_poly(conn, *pid, *black, XCB_POLY_SHAPE_CONVEX, XCB_COORD_MODE_ORIGIN, 4, top_right_triangle);
}

/*
 * Apply shape to bottom corners when possible
 */
void triangled_bottom(Con *con, xcb_pixmap_t *pid, xcb_gcontext_t *black) {

    int32_t size = config.corners.size;
    uint16_t w  = con->rect.width;
    uint16_t h  = con->rect.height;

    xcb_point_t bottom_left_triangle[] = {
        {0, h-size}, {size, h}, {0, h}, {0, h-size}
    };

    xcb_point_t bottom_right_triangle[] = {
        {w, h-size}, {w, h}, {w-size, h}, {w, h-size}
    };

    xcb_fill_poly(conn, *pid, *black, XCB_POLY_SHAPE_CONVEX, XCB_COORD_MODE_ORIGIN, 4, bottom_left_triangle);
    xcb_fill_poly(conn, *pid, *black, XCB_POLY_SHAPE_CONVEX, XCB_COORD_MODE_ORIGIN, 4, bottom_right_triangle);

}

void x_shape_window(Con *con) {

    const xcb_query_extension_reply_t *shape_query;
    shape_query = xcb_get_extension_data(conn, &xcb_shape_id);

    if (!shape_query->present || con->parent->type == CT_DOCKAREA) {
        return;
    }

    if (con->fullscreen_mode) {
        xcb_shape_mask(conn, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_BOUNDING, con->frame.id, 0, 0, XCB_NONE);
        xcb_shape_mask(conn, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_CLIP, con->frame.id, 0, 0, XCB_NONE);
        return;
    }

    xcb_pixmap_t pid = xcb_generate_id(conn);
    xcb_create_pixmap(conn, 1, pid, con->frame.id, con->rect.width, con->rect.height);

    xcb_gcontext_t white = xcb_generate_id(conn);
    xcb_gcontext_t black = xcb_generate_id(conn);

    xcb_create_gc(conn, white, pid, XCB_GC_FOREGROUND, (uint32_t[]){1, 0});
    xcb_create_gc(conn, black, pid, XCB_GC_FOREGROUND, (uint32_t[]){0, 0});

    uint16_t w  = con->rect.width;
    uint16_t h  = con->rect.height;

    xcb_rectangle_t bounding = {0, 0, w, h};
    xcb_poly_fill_rectangle(conn, pid, white, 1, &bounding);

    bool tab_or_stack = false;
    for (Con* current = con; current->parent->type != CT_WORKSPACE; current = current->parent)
    {
        if ((current->layout == L_STACKED|| current->layout == L_TABBED))
            tab_or_stack = true;
    }

    if (con->parent->type == CT_WORKSPACE || !tab_or_stack) {
        if (config.corners.shape == ROUNDED_CORNERS) {
            rounded_top(con, &pid, &black);
            rounded_bottom(con, &pid, &black);
        } else if (config.corners.shape == TRIANGULAR_CORNERS){
            triangled_top(con, &pid, &black);
            triangled_bottom(con, &pid, &black);
        } else {
          DLOG("corners %d, %d\n", config.corners.size, config.corners.shape);
        }
    }

    else {
        if (config.corners.shape == ROUNDED_CORNERS) {
            rounded_bottom(con, &pid, &black);
        } else if (config.corners.shape == TRIANGULAR_CORNERS){
            triangled_bottom(con, &pid, &black);
        } else {
          DLOG("corners %d, %d\n", config.corners.size, config.corners.shape);
        }
    }

    xcb_shape_mask(conn, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_BOUNDING, con->frame.id, 0, 0, pid);
    xcb_shape_mask(conn, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_CLIP, con->frame.id, 0, 0, pid);

    xcb_free_pixmap(conn, pid);
}

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
void rounded_top(Con *con, xcb_pixmap_t *pid, xcb_gcontext_t *white, int size) {

    uint16_t w  = con->rect.width;
    uint16_t h  = con->rect.height;

    int32_t d = 2 * size;

    xcb_arc_t arcs[] = {
      { 0, 0, d, d, 90 << 6, 90 << 6},
      { w-d, 0, d, d, 0 << 6 , 90 << 6 },
    };

    xcb_poly_fill_arc(conn, *pid, *white, 2, arcs);
}

/*
 * Apply shape to bottom corners when possible
 */
void rounded_bottom(Con *con, xcb_pixmap_t *pid, xcb_gcontext_t *white, int size) {

    uint16_t w  = con->rect.width;
    uint16_t h  = con->rect.height;

    int32_t d = 2 * size;

    xcb_arc_t arcs[] = {
       { 0, h-d, d, d, 180 << 6, 90 << 6 },
       { w-d, h-d, d, d, 270 << 6, 90 << 6 }
    };

    xcb_poly_fill_arc(conn, *pid, *white, 2, arcs);
}

/*
 * Apply shape to top corners when possible
 */
void triangled_top(Con *con, xcb_pixmap_t *pid, xcb_gcontext_t *white, int size) {

    uint16_t w  = con->rect.width;
    uint16_t h  = con->rect.height;


    xcb_point_t top_left_triangle[] = {
        {size, size}, {size, 0}, {0, size}, {size, size}
    };

    xcb_point_t top_right_triangle[] = {
        {w-size, 0}, {w, size}, {w-size, size}, {w-size, 0}
    };

    xcb_fill_poly(conn, *pid, *white, XCB_POLY_SHAPE_CONVEX, XCB_COORD_MODE_ORIGIN, 4, top_left_triangle);
    xcb_fill_poly(conn, *pid, *white, XCB_POLY_SHAPE_CONVEX, XCB_COORD_MODE_ORIGIN, 4, top_right_triangle);
}

/*
 * Apply shape to bottom corners when possible
 */
void triangled_bottom(Con *con, xcb_pixmap_t *pid, xcb_gcontext_t *white, int size) {

    uint16_t w  = con->rect.width;
    uint16_t h  = con->rect.height;

    xcb_point_t bottom_left_triangle[] = {
        {0, h-size}, {size, h-size}, {size, h}, {0, h-size}
    };

    xcb_point_t bottom_right_triangle[] = {
        {w, h-size}, {w-size, h}, {w-size, h-size}, {w, h-size}
    };

    xcb_fill_poly(conn, *pid, *white, XCB_POLY_SHAPE_CONVEX, XCB_COORD_MODE_ORIGIN, 4, bottom_left_triangle);
    xcb_fill_poly(conn, *pid, *white, XCB_POLY_SHAPE_CONVEX, XCB_COORD_MODE_ORIGIN, 4, bottom_right_triangle);

}

/*
 * Apply shape to bottom corners when possible
 */
void fill_top(Con *con, xcb_pixmap_t *pid, xcb_gcontext_t *white, int size) {

    uint16_t w  = con->rect.width;
    uint16_t h  = con->rect.height;

    xcb_rectangle_t top_left_rectangle = {0, 0, size, size};
    xcb_rectangle_t top_right_rectangle = {w-size, 0, size, size};

    xcb_poly_fill_rectangle(conn, *pid, *white, 1, &top_left_rectangle);
    xcb_poly_fill_rectangle(conn, *pid, *white, 1, &top_right_rectangle);
}

/*
 * Apply shape to bottom corners when possible
 */
void fill_bottom(Con *con, xcb_pixmap_t *pid, xcb_gcontext_t *white, int size) {

    uint16_t w  = con->rect.width;
    uint16_t h  = con->rect.height;

    xcb_rectangle_t bottom_left_rectangle = {0, h-size, size, size};
    xcb_rectangle_t bottom_right_rectangle = {w-size, h-size, size, size};

    xcb_poly_fill_rectangle(conn, *pid, *white, 1, &bottom_left_rectangle);
    xcb_poly_fill_rectangle(conn, *pid, *white, 1, &bottom_right_rectangle);
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

   Con* workspace = con_get_workspace(con);
    uint16_t w  = con->rect.width;
    uint16_t h  = con->rect.height;
    int32_t size = workspace->corners.size;

    // Outer box, the rectangle window
    xcb_rectangle_t outer_box = {0, 0, w, h};
    // Inner box, without corners
    xcb_point_t inner_box[] = {
        {0, size}, {size, size}, {size, 0}, {w-size, 0}, {w-size, size}, {w, size},
        {w, h-size}, {w-size, h-size}, {w-size, h}, {size, h}, {size, h-size}, {0, h-size}
    };

    // Fill the boxes
    xcb_poly_fill_rectangle(conn, pid, black, 1, &outer_box);
    xcb_fill_poly(conn, pid, white, XCB_POLY_SHAPE_CONVEX, XCB_COORD_MODE_ORIGIN, 12, inner_box);


    bool tab_or_stack = false;
    for (Con* current = con; current->parent->type != CT_WORKSPACE; current = current->parent)
    {
        if ((current->layout == L_STACKED|| current->layout == L_TABBED))
            tab_or_stack = true;
    }



    if (con->parent->type == CT_WORKSPACE || !tab_or_stack) {
        if (workspace->corners.shape == DEFAULT_CORNERS) {
            fill_top(con, &pid, &white, workspace->corners.size);
            fill_bottom(con, &pid, &white, workspace->corners.size);
        } else if (workspace->corners.shape == ROUNDED_CORNERS) {
            rounded_top(con, &pid, &white, workspace->corners.size);
            rounded_bottom(con, &pid, &white, workspace->corners.size);
        } else if (workspace->corners.shape == TRIANGULAR_CORNERS){
            triangled_top(con, &pid, &white, workspace->corners.size);
            triangled_bottom(con, &pid, &white, workspace->corners.size);
         } else if (workspace->corners.shape == TRIMMED_CORNERS){
       } else {
          DLOG("corners %d, %d\n", workspace->corners.size, workspace->corners.shape);
        }
    }

    else {
        if (workspace->corners.shape == ROUNDED_CORNERS) {
            rounded_bottom(con, &pid, &white, workspace->corners.size);
        } else if (workspace->corners.shape == TRIANGULAR_CORNERS){
            triangled_bottom(con, &pid, &white, workspace->corners.size);
         } else if (workspace->corners.shape == TRIMMED_CORNERS){
            fill_top(con, &pid, &white, workspace->corners.size);
        } else {
          DLOG("corners %d, %d\n", workspace->corners.size, workspace->corners.shape);
        }
    }

    xcb_shape_mask(conn, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_BOUNDING, con->frame.id, 0, 0, pid);
    xcb_shape_mask(conn, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_CLIP, con->frame.id, 0, 0, pid);

    xcb_free_pixmap(conn, pid);
}

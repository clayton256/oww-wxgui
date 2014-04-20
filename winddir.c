

static void rotate_points(int vane, GdkPoint *point, int offx, int offy, int N)
{
    /*  Rotate coordinates by the given vane position, and adds offset */

    GdkPoint out ;

    long int sinval[] = {0,
      0,
      25080,
      46341,
      60547,
      65536,
      60547,
      46341,
      25080,
      0,
      -25080,
      -46341,
      -60547,
      -65536,
      -60547,
      -46341,
      -25080
      } ;

    long int cosval[] = {0,
      -65536,
      -60547,
      -46341,
      -25080,
      0,
      25080,
      46341,
      60547,
      65536,
      60547,
      46341,
      25080,
      0,
      -25080,
      -46341,
      -60547
      } ;

    int i ;
    long int out_x, out_y ;

    for (i=0; i<N; ++i) {
        out_x = (long int) point[i].x * cosval[vane] + (long int) point[i].y * sinval[vane] ;
        out.x = (int) offx + (out_x >> 16) ;
        out_y = (long int) point[i].x * -sinval[vane] + (long int) point[i].y * cosval[vane] ;
        out.y = (int) offy +  (out_y >> 16) ;
        /* printf("%d,%d -> %d,%d\n", point[i].x,point[i].y,out.x,out.y);*/
        point[i] = out ;
    }

}

static void draw_string_centred(gchar *string, GdkGC *style, gint x, gint y)
{
    gdk_draw_string(pixmap, font, style, x - gdk_string_width(font, string)/2,
            y + gdk_string_height(font, string)/2, string) ;
}

static void
mainwin_draw_vane_ring(int new_data, int vane, int xc, int yc, int radius)
{
    /*  Keeping a record of vane positions, draw a grey-scale ring */

    int i ;
    static int vp[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
               pc[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} ;
    static int j = 0 ;

    if (new_data)
    {
        if ((vane < 1) || (vane > 16))
        {
            werr(WERR_DEBUG0, "Bad vane value: %d", vane) ;
            return ;
        }

        /*  vane runs from 1 to 16 */
        /*  Decrement count for old vane position */
        if ((vp[j] > 0) && (pc[vp[j]-1] > 0)) --pc[vp[j]-1] ;

        /*  Note this position */
        vp[j] = vane ;

        /*  Increment count for this position */
        ++pc[vane-1] ;

        /*  Increment round-robin index */
        j = (j+1)%15 ;
    }

    /*  Now draw the arcs for each position */
    for (i=0; i<16; ++i)
    {
        /*  Unfilled arcs */
        gdk_draw_arc(pixmap, VaneArc[pc[i]], 1,
                     xc-radius, yc-radius, 2*radius, 2*radius, (4 - i) * 360 * 4 - 720, 1440) ;
    }
}

static void mainwin_draw_compass_rose(int new_data, int vane, GdkRectangle area)
{
    #define BOSS_RAD 18

    gint xc, yc, i ;

    if (!devices_have_vane()) return;

    /* rotate_points_reset() ;*/

    xc = area.x + area.width/2 ;
    yc = area.y + area.height/2 ;

    /*  Vane Arcs */
    mainwin_draw_vane_ring(new_data, vane, xc, yc, BOSS_RAD+7) ;

    /*  Draw central boss */

    /*  Filled circle */
    gdk_draw_arc(pixmap, penWhite, 1,
                 xc-BOSS_RAD, yc-BOSS_RAD, 2*BOSS_RAD, 2*BOSS_RAD, 0, 360 * 64) ;

    /*  Set line attributes */
    gdk_gc_set_line_attributes(penFatGreen, 8,
                              GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;
    gdk_gc_set_line_attributes(penFatWhite, 8,
                                GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;

    /*  Text labels */
    for (i=0; i<8; ++i) {
        gchar *label[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"} ;
        gint pos ;
        GdkPoint tp = {0, 67} ;
        if (i%2) tp.y -= 4 ;

        pos = 1 + 2*i ;
        rotate_points(pos, &tp, xc, yc, 1) ;
        draw_string_centred(label[i], (pos==vane)? penGreen : penWhite, tp.x, tp.y) ;
    }

    /*  Cardinal points - fat rounded lines */
    for (i=1; i<17; i+=4) {
        GdkPoint line[2] = {{0,29}, {0,51}} ;
        rotate_points(i, line, xc, yc, 2) ;
        gdk_draw_lines(pixmap, (i==vane)? penFatGreen : penFatWhite, line, 2) ;
    }

    /*  Set line attributes */
    gdk_gc_set_line_attributes(penFatGreen, 12, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;
    gdk_gc_set_line_attributes(penFatWhite, 12, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;

    /*  NE, SE, SW, NW - fat points */
    for (i=3; i<17; i+=4) {
        GdkPoint points[2] = {{0,33}, {0,33}} ;

        rotate_points(i, points, xc, yc, 2) ;
        gdk_draw_lines(pixmap, (i==vane)? penFatGreen : penFatWhite, points, 2) ;
    }

    /*  Set line attributes */
    gdk_gc_set_line_attributes(penFatGreen, 7, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;
    gdk_gc_set_line_attributes(penFatWhite, 7, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;

    /*  minor points */
    for (i=2; i<17; i+=2) {
        GdkPoint points[2] = {{0,31}, {0,31}} ;

        rotate_points(i, points, xc, yc, 2) ;
        gdk_draw_lines(pixmap, (i==vane)? penFatGreen : penFatWhite, points, 2) ;
    }

    /*  Pointer */
    if (vane) {
        GdkPoint stem[] = {{0,BOSS_RAD}, {0,-BOSS_RAD}} ;
        GdkPoint head[] = {{-8,10}, {0,18}, {8,10}} ;

        rotate_points(vane, stem, xc, yc, 2) ;
        rotate_points(vane, head, xc, yc, 3) ;

        gdk_draw_lines(pixmap, penFatBlack, stem, 2) ;
        gdk_draw_polygon(pixmap, penBlack, 1, head, 3) ;
    }
}


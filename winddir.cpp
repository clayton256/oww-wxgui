
#include <wx/pen.h>
#include <wx/brush.h>

#include "winddir.h"

static void rotate_points(int vane, wxPoint *point, int offx, int offy, int N)
{
    /*  Rotate coordinates by the given vane position, and adds offset */

    wxPoint out ;

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

static void draw_string_centred(wxPaintDC *d, const char *string, const wxColour *style, int x, int y)
{
	wxPoint pt;
	int string_width, string_height;
    //wxFont f = wxFont(m_frame->m_fontSz, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, 
    //                                                    wxFONTWEIGHT_NORMAL);
	d->GetTextExtent(string, &string_width, &string_height);
	pt.x = x - (string_width/2);
    pt.y = y - (string_height/2);
    wxColor fc = d->GetTextForeground();
    //d->SetFont(f);
    d->SetTextForeground( wxT("BLACK") );
    d->DrawText( string, pt.x+2, pt.y+2);
    d->SetTextForeground( wxT("YELLOW") );
    d->DrawText( string, pt.x, pt.y);
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
            //werr(WERR_DEBUG0, "Bad vane value: %d", vane) ;
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
		//dc->DrawArc(&dc, 
        //gdk_draw_arc(VaneArc[pc[i]], 1,
        //             xc-radius, yc-radius, 2*radius, 2*radius, (4 - i) * 360 * 4 - 720, 1440) ;
    }
}


void mainwin_draw_compass_rose(wxPaintDC *dc, int new_data, int vane, wxRectangle area)
{
    #define BOSS_RAD 18

    int xc, yc, i ;

    /* rotate_points_reset() ;*/

    xc = area.x + area.width/2 ;
    yc = area.y + area.height/2 ;

    /*  Vane Arcs */
    mainwin_draw_vane_ring(new_data, vane, xc, yc, BOSS_RAD+7) ;

    /*  Draw central boss */

    /*  Filled circle */
    dc->SetBrush( wxBrush( wxT("green"), wxSOLID ) );
    //dc->DrawCircle(xc-BOSS_RAD, yc-BOSS_RAD, 2*BOSS_RAD);
    dc->DrawCircle(xc, yc, 2*BOSS_RAD);

    /*  Set line attributes */
    //gdk_gc_set_line_attributes(penFatGreen, 8,
    //                          GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;
    //gdk_gc_set_line_attributes(penFatWhite, 8,
    //                            GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;

    /*  Text labels */
    for (i=0; i<8; ++i) {
        const char *label[8] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"} ;
        int pos ;
        wxPoint tp(0, 55);
        if (i%2) tp.y -= 4 ;

        pos = 1 + 2*i ;
        rotate_points(pos, &tp, xc, yc, 1) ;
        draw_string_centred(dc, label[i], (pos==vane)? wxGREEN : wxWHITE, tp.x, tp.y) ;
    }

    /*  Cardinal points - fat rounded lines */
    for (i=1; i<17; i+=4) {
        wxPoint line[2];
        line[0].x = 0; line[0].y = 29; 
        line[1].x = 0; line[1].y = 36;
        rotate_points(i, line, xc, yc, 2) ;
		wxPen pen((i==vane)? *wxGREEN : *wxWHITE, 9);
        dc->SetPen(pen);
		dc->DrawLine( line[0], line[1]);
    }

    /*  Set line attributes */
    //gdk_gc_set_line_attributes(penFatGreen, 12, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;
    //gdk_gc_set_line_attributes(penFatWhite, 12, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;

    /*  NE, SE, SW, NW - fat points */
    for (i=3; i<17; i+=4) {
        wxPoint points[2];
        points[0].x = 0; points[0].y = 33; 
        points[1].x = 0; points[1].y = 33;

        rotate_points(i, points, xc, yc, 2) ;
		wxPen pen((i==vane)? *wxGREEN : *wxWHITE, 9);
        dc->SetPen(pen);
		dc->DrawLine( points[0], points[1]);
    }

    /*  Set line attributes */
    //gdk_gc_set_line_attributes(penFatGreen, 7, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;
    //gdk_gc_set_line_attributes(penFatWhite, 7, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_MITER) ;

    /*  minor points */
    for (i=2; i<17; i+=2) {
        wxPoint points[2];
        points[0].x = 0; points[0].y = 31; 
        points[1].x = 0; points[1].y = 31;

        rotate_points(i, points, xc, yc, 2) ;
		wxPen pen((i==vane)? *wxGREEN : *wxWHITE, 5);
        dc->SetPen(pen);
		dc->DrawLine( points[0], points[1]);
    }

    /*  Pointer */
    if (vane) {
        wxPoint stem[2];
        wxPoint head[3];
        stem[0].x = 0; stem[0].y = BOSS_RAD; 
        stem[1].x = 0; stem[1].y = -BOSS_RAD;
        head[0].x = -8; head[0].y = 10; 
        head[1].x = 0; head[1].y = 18;
        head[2].x = 8; head[2].y = 10; 

        rotate_points(vane, stem, xc, yc, 2) ;
        rotate_points(vane, head, xc, yc, 3) ;

		wxPen penFatBlack(*wxBLACK, 9);
        dc->SetPen(penFatBlack);
		dc->DrawLine( stem[0], stem[1]);
		wxPen penBlack(*wxBLACK, 4);
        dc->SetPen(penBlack);
		dc->DrawPolygon(3, head);
    }
}


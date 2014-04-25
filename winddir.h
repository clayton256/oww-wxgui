
#include <wx/dcclient.h>
#include <wx/gdicmn.h>

class wxRectangle
{
	public:
	int x;
	int y;
	int width;
	int height;
};



void mainwin_draw_compass_rose(wxPaintDC *dc, int new_data, int vane, wxRectangle area);




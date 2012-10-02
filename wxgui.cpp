///////////////////////////////////////////////////////////////////////////////
// Name:        
// Purpose:     
// Author:      
// Created:     
// Copyright:   (c) 
// Licence:     
///////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/image.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/wfstream.h>
#include <wx/quantize.h>
#include <wx/stopwatch.h>
#include <wx/graphics.h>

#include "wxgui.h"

wxString g_VersionStr = VERSIONSTR;

#ifndef wxHAS_IMAGES_IN_RESOURCES
    #include "pixmaps/oww_xpm.xpm"
#endif

// ============================================================================
// declarations
// ============================================================================

//-----------------------------------------------------------------------------
// MyApp
//-----------------------------------------------------------------------------

class MyApp: public wxApp
{
public:
    virtual bool OnInit();
};

// ----------------------------------------------------------------------------
// MyFrame
// ----------------------------------------------------------------------------

class MyFrame: public wxFrame
{
    RenderTimer* timer;

public:
    MyFrame();

    void OnAbout( wxCommandEvent &event );
    void OnAuxilliary( wxCommandEvent &event );
    void OnMap( wxCommandEvent &event );
    void OnMessages( wxCommandEvent &event );
    void OnSetup( wxCommandEvent &event );
    void OnDevices( wxCommandEvent &event );
    void OnQuit( wxCommandEvent &event );

    MyCanvas         *m_canvas;
    wxStatusBar      *m_statusbar;
private:

    DECLARE_DYNAMIC_CLASS(MyFrame)
    DECLARE_EVENT_TABLE()
};


class MyImageFrame : public wxFrame
{
public:
    MyImageFrame(wxFrame *parent, const wxString& desc)
    {
        Create(parent, desc); 
    }

private:
    bool Create(wxFrame *parent, const wxString& desc)
    {
        if ( !wxFrame::Create(parent, wxID_ANY,
                              wxString::Format(wxT("Image from %s"), desc),
                              wxDefaultPosition, wxDefaultSize,
                              wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE) )
            return false;

        wxMenu *menu = new wxMenu;
        menu->Append(wxID_SAVE);

        wxMenuBar *mbar = new wxMenuBar;
        mbar->Append(menu, wxT("&Image"));
        SetMenuBar(mbar);

        CreateStatusBar(2);
        SetStatusText("Loading", 1);

        UpdateStatusBar();

        Show();

        return true;
    }

    void OnEraseBackground(wxEraseEvent& WXUNUSED(event))
    {
        // do nothing here to be able to see how transparent images are shown
    }

    void OnPaint(wxPaintEvent& WXUNUSED(event))
    {
        wxPaintDC dc(this);

        const wxSize size = GetClientSize();
    }

    void OnSave(wxCommandEvent& WXUNUSED(event))
    {
    }

    void UpdateStatusBar()
    {
        wxLogStatus(this, wxT("Image size: (%d, %d), zoom %.2f"), 5, 10, 22.2 );
        Refresh();
    }


    DECLARE_EVENT_TABLE()
};


// ============================================================================
// implementations
// ============================================================================

//-----------------------------------------------------------------------------
// MyImageFrame
//-----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MyImageFrame, wxFrame)
    EVT_ERASE_BACKGROUND(MyImageFrame::OnEraseBackground)
    EVT_PAINT(MyImageFrame::OnPaint)

    EVT_MENU(wxID_SAVE, MyImageFrame::OnSave)
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
// MyFrame
//-----------------------------------------------------------------------------

enum
{
    ID_QUIT  = wxID_EXIT,
    ID_ABOUT = wxID_ABOUT,
    ID_AUXILLIARY = 100,
    ID_MAP,
    ID_MESSAGES,
    ID_SETUP,
    ID_DEVICES
};

IMPLEMENT_DYNAMIC_CLASS( MyFrame, wxFrame )
BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU    (ID_ABOUT, MyFrame::OnAbout)
    EVT_MENU    (ID_QUIT,  MyFrame::OnQuit)
    EVT_MENU    (ID_AUXILLIARY, MyFrame::OnAuxilliary)
    EVT_MENU    (ID_MAP,   MyFrame::OnMap)
    EVT_MENU    (ID_MESSAGES,  MyFrame::OnMessages)
    EVT_MENU    (ID_SETUP, MyFrame::OnSetup)
    EVT_MENU    (ID_DEVICES, MyFrame::OnDevices)
END_EVENT_TABLE()

MyFrame::MyFrame()
    : wxFrame( (wxFrame *)NULL, wxID_ANY, wxT("Oww"),
                wxPoint(20, 20), 
#ifdef __WXGTK__
                wxSize(474, 441+30)
#else
                wxSize(474, 441)
#endif
              )
{
    SetIcon(wxICON(oww));

    wxMenuBar *menu_bar = new wxMenuBar();

    wxMenu *menuImage = new wxMenu;
    menuImage->Append( ID_AUXILLIARY, wxT("Auxilary"), "See other device values");
    menuImage->Append( ID_MESSAGES, wxT("Messages"), "See running log");
    menuImage->Append( ID_MAP, wxT("Map"), "Map this station");
    menuImage->AppendSeparator();
    menuImage->Append( ID_SETUP, wxT("Setup"), "Preferences");
    menuImage->Append(ID_DEVICES, "Devices", "Configure devices");
    menuImage->AppendSeparator();
    menuImage->Append( ID_ABOUT, wxT("&About"));
    menuImage->Append( ID_ABOUT, wxT("&Help"));
    menuImage->AppendSeparator();
    menuImage->Append( ID_QUIT, wxT("E&xit\tCtrl-Q"));
    menu_bar->Append(menuImage, wxT("Menu"));

    SetMenuBar( menu_bar );

    m_statusbar = CreateStatusBar(2);
    int widths[] = { -1, 100 };
    SetStatusWidths( 2, widths );
    SetStatusText("Hi There!", 1);
    wxLogStatus(this, wxT(": (%d, %d), %.2f"), 5, 10, 22.2 );
    Refresh();

    SetTitle(GetTitle() + "://" + "192.168.1.22" + ":" + "8080");

    m_canvas = new MyCanvas( this, wxID_ANY, wxPoint(0,0), wxSize(10,10) );

    timer = new RenderTimer(m_canvas);
    Show();
    timer->start();
 
 
    // 500 width * 2750 height
    //m_canvas->SetScrollbars( 10, 10, 50, 275 );
}

void MyFrame::OnQuit( wxCommandEvent &WXUNUSED(event) )
{
    timer->Stop();
    delete timer;
    Close( true );
}


void MyFrame::OnAbout( wxCommandEvent &WXUNUSED(event) )
{
    wxArrayString array;

    array.Add("Oww");
    array.Add("One wire weather");
    array.Add("(c) Dr. Simon J. Melhuish");

    array.Add(wxEmptyString);
    array.Add("Version: " + g_VersionStr);
    //array.Add("Version of g++: " + __VERSION__);

    (void)wxMessageBox( wxJoin(array, '\n'),
                        "One wire Weather",
                        wxICON_INFORMATION | wxOK );
}

class MyGraphicsFrame : public wxFrame
{
public:
    enum
    {
        WIDTH = 256,
        HEIGHT = 90
    };

    MyGraphicsFrame(wxWindow* parent, wxString title) :
        wxFrame(parent, wxID_ANY, title)
    {

        Connect(wxEVT_PAINT, wxPaintEventHandler(MyGraphicsFrame::OnPaint));

        Show();
    }

private:
    void OnPaint(wxPaintEvent& WXUNUSED(event))
    {
        wxPaintDC dc(this);
        wxScopedPtr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));

        gc->SetFont(*wxNORMAL_FONT, *wxBLACK);
        gc->DrawText("Text Here", 0, HEIGHT/2);

        wxGraphicsFont gf = gc->CreateFont(wxNORMAL_FONT->GetPixelSize().y, "");
        gc->SetFont(gf);
        gc->DrawText("More Text", 0, (3*HEIGHT)/2);
    }

    wxDECLARE_NO_COPY_CLASS(MyGraphicsFrame);
};



BEGIN_EVENT_TABLE(MyBasicDialog, wxDialog)
    EVT_BUTTON(wxID_ANY, MyBasicDialog::OnOk)
END_EVENT_TABLE()



void MyBasicDialog::OnOk (wxCommandEvent & event)
{
    Close(true);
    return;
}

wxString MyBasicDialog::GetText ()
{
    return "Goober!!!";
}

//MyBasicDialog::~MyBasicDialog ()
//{
//}
 
MyBasicDialog::MyBasicDialog ( wxWindow * parent, wxWindowID id, 
                                wxString const & title,
	              wxPoint const & position = wxDefaultPosition,
	              wxSize const & size = wxDefaultSize,
	              long style = wxDEFAULT_DIALOG_STYLE )
: wxDialog( parent, id, title, position, size, style)
{
	wxString dimensions = "", s;
	wxPoint p;
	wxSize  sz;
 
	sz.SetWidth(size.GetWidth() - 20);
	sz.SetHeight(size.GetHeight() - 70);
 
	p.x = 6; p.y = 2;
	s.Printf(_(" x = %d y = %d\n"), p.x, p.y);
	dimensions.append(s);
	s.Printf(_(" width = %d height = %d\n"), sz.GetWidth(), sz.GetHeight());
	dimensions.append(s);
	dimensions.append("Mark Clayton");
 
	dialogText = new wxTextCtrl(this, -1, dimensions, p, sz, wxTE_MULTILINE);
 
	p.y += sz.GetHeight() + 10;
	wxButton * b = new wxButton(this, wxID_OK, _("OK"), p, wxDefaultSize);
	p.x += 110;
	wxButton * c = new wxButton(this, wxID_CANCEL, _("Cancel"), p, wxDefaultSize);
}

void MyFrame::OnSetup( wxCommandEvent &WXUNUSED(event) )
{
    wxString dialogText;
    MyBasicDialog setupDialog(this, -1, _("Your very own dialog"),
	                          wxPoint(100, 100), wxSize(200, 200));
	if ( setupDialog.ShowModal() != wxID_OK ) /* Modal .ShowModal() vs non-Modal .Show() */
		dialogText = "The about box was cancelled.\n";
	else
		dialogText = setupDialog.GetText();
}

void MyFrame::OnAuxilliary(wxCommandEvent &WXUNUSED(event))
{
    new MyImageFrame(this, "Aux");
}

void MyFrame::OnMap( wxCommandEvent &WXUNUSED(event) )
{
    new MyImageFrame(this, "Map");
}


void MyFrame::OnMessages( wxCommandEvent &WXUNUSED(event) )
{
    new MyGraphicsFrame(this, "Messages");
}


void MyFrame::OnDevices(wxCommandEvent& WXUNUSED(event))
{
    new MyGraphicsFrame(this, "Devices");
}




//-----------------------------------------------------------------------------
// MyApp
//-----------------------------------------------------------------------------

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
    if ( !wxApp::OnInit() )
        return false;

    wxInitAllImageHandlers();

    wxFrame *frame = new MyFrame();
    frame->Show( true );

    return true;
}




 
RenderTimer::RenderTimer(MyCanvas* pane) : wxTimer()
{
    RenderTimer::canvas = pane;
}
 
void RenderTimer::Notify()
{
    canvas->Refresh();
}
 
void RenderTimer::start()
{
    wxTimer::Start(40);
}
 
 
 
//-----------------------------------------------------------------------------
// MyCanvas
//-----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MyCanvas, wxPanel)
    EVT_PAINT(MyCanvas::OnPaint)
END_EVENT_TABLE()

MyCanvas::MyCanvas( wxWindow *parent, wxWindowID id,
                    const wxPoint &pos, const wxSize &size )
    : wxPanel( parent, id, pos, size, wxSUNKEN_BORDER )
{
    wxImage image;
    SetBackgroundColour(*wxWHITE);
/*
    wxBitmap bitmap( 100, 100 );

    wxMemoryDC dc;
    dc.SelectObject( bitmap );
    dc.SetBrush( wxBrush( wxT("orange"), wxSOLID ) );
    dc.SetPen( *wxBLACK_PEN );
    dc.DrawRectangle( 0, 0, 100, 100 );
    dc.SetBrush( *wxWHITE_BRUSH );
    dc.DrawRectangle( 20, 20, 60, 60 );
    dc.SelectObject( wxNullBitmap );
*/
    // try to find the directory with our images
    wxString dir = "./pixmaps/";
    if ( wxFile::Exists( dir + wxT("body.jpg"))
      && wxFile::Exists( dir + wxT("top1.jpg")) 
      && wxFile::Exists( dir + wxT("top2.jpg")) 
      && wxFile::Exists( dir + wxT("top3.jpg")) 
      && wxFile::Exists( dir + wxT("bottom1.jpg")) 
      && wxFile::Exists( dir + wxT("bottom2.jpg")) 
      && wxFile::Exists( dir + wxT("bottom3.jpg")) 
      && wxFile::Exists( dir + wxT("bottom4.jpg")) 
      && wxFile::Exists( dir + wxT("bottom5.jpg")) 
      && wxFile::Exists( dir + wxT("bottom6.jpg")) 
      && wxFile::Exists( dir + wxT("bottom7.jpg")) 
      && wxFile::Exists( dir + wxT("bottom8.jpg")) 
      && wxFile::Exists( dir + wxT("rh.png")) )
    {
        // found image files
    }
    else
        wxLogWarning(wxT("Can't find image files!"));

    if ( !image.LoadFile( dir + wxString(_T("body.jpg"))) )
        wxLogError(_T("Can't load JPG image"));
    else
        body_jpg = wxBitmap( image );

    if ( !image.LoadFile( dir + wxString(_T("bottom1.jpg"))) )
        wxLogError(_T("Can't load JPG image"));
    else
        bottom1_jpg = wxBitmap( image );

    if ( !image.LoadFile( dir + wxString(_T("bottom2.jpg"))) )
        wxLogError(_T("Can't load JPG image"));
    else
        bottom2_jpg = wxBitmap( image );

    if ( !image.LoadFile( dir + wxString(_T("bottom3.jpg"))) )
        wxLogError(_T("Can't load JPG image"));
    else
        bottom3_jpg = wxBitmap( image );

    if ( !image.LoadFile( dir + wxString(_T("bottom4.jpg"))) )
        wxLogError(_T("Can't load JPG image"));
    else
        bottom4_jpg = wxBitmap( image );

    if ( !image.LoadFile( dir + wxString(_T("bottom5.jpg"))) )
        wxLogError(_T("Can't load JPG image"));
    else
        bottom5_jpg = wxBitmap( image );

    if ( !image.LoadFile( dir + wxString(_T("bottom6.jpg"))) )
        wxLogError(_T("Can't load JPG image"));
    else
        bottom6_jpg = wxBitmap( image );

    if ( !image.LoadFile( dir + wxString(_T("bottom7.jpg"))) )
        wxLogError(_T("Can't load JPG image"));
    else
        bottom7_jpg = wxBitmap( image );

    if ( !image.LoadFile( dir + wxString(_T("bottom8.jpg"))) )
        wxLogError(_T("Can't load JPG image"));
    else
        bottom8_jpg = wxBitmap( image );

    if ( !image.LoadFile( dir + wxString(_T("rh.png"))) )
        wxLogError(_T("Can't load PNG image"));
    else
        rh_png = wxBitmap( image );

    if ( !image.LoadFile( dir + wxString(_T("top1.jpg"))) )
        wxLogError(_T("Can't load JPG image"));
    else
        top1_jpg = wxBitmap( image );

    if ( !image.LoadFile( dir + wxString(_T("top2.jpg"))) )
        wxLogError(_T("Can't load JPG image"));
    else
        top2_jpg = wxBitmap( image );

    if ( !image.LoadFile( dir + wxString(_T("top3.jpg"))) )
        wxLogError(_T("Can't load JPG image"));
    else
        top3_jpg = wxBitmap( image );


}

MyCanvas::~MyCanvas()
{
}

int counter = 0;

#include <wx/utils.h> 

void MyCanvas::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
    wxPaintDC dc( this );
    PrepareDC( dc );

    switch (counter % 3)
    {
        case 0:
	    if (top1_jpg.IsOk())
	    {
		dc.DrawBitmap( top1_jpg, 0, 0 );
	    }
            counter++;
            break;

        case 1:
	    if (top2_jpg.IsOk())
	    {
		dc.DrawBitmap( top2_jpg, 0, 0 );
	    }
            counter++;
            break;
        case 2:
	    if (top3_jpg.IsOk())
	    {
		dc.DrawBitmap( top3_jpg, 0, 0 );
	    }
            counter = 0;
    }    

    if (body_jpg.IsOk())
        dc.DrawBitmap( body_jpg, 0, top1_jpg.GetHeight()); //134 );

    if (bottom1_jpg.IsOk())
        dc.DrawBitmap( bottom8_jpg, 0, top1_jpg.GetHeight() + body_jpg.GetHeight()); //196 );

    if (rh_png.IsOk())
        dc.DrawBitmap( rh_png, 300, 180 );


    dc.SetTextForeground( wxT("WHITE") );
    wxString now = wxNow ();
    dc.DrawText (now, 100, 10);
    dc.DrawText( wxT("-32.4F"), 30, 135 );

    dc.DrawText( wxT("22 mph"), 365, 20 );
    dc.SetBrush( wxBrush( wxT("white"), wxSOLID ) );
    dc.SetPen( *wxBLACK_PEN );
    dc.DrawCircle( 350, 100, 30);



    //dc.SetBrush( *wxWHITE_BRUSH );
    //dc.SetPen( *wxRED_PEN );
    //dc.DrawRectangle( 170, 50, 60, 60 );

}


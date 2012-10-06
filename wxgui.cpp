///////////////////////////////////////////////////////////////////////////////
// Name:        wxgui.cpp
// Purpose:     wxGui classes and implimentations.
// Author:      Mark Clayton <mark_clayton@users.sourceforge.net>
// Created:     Sept, 30 2012
// Copyright:   (c) John Mark Clayton
// Licence:     GPL Ver 3: See the COPYING file that came with this package
///////////////////////////////////////////////////////////////////////////////
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
#include <wx/grid.h>
#include <wx/listctrl.h>
#include <wx/config.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "wxgui.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "owwl.h"
#ifdef __cplusplus
} /*extern "C"*/
#endif

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
    OwwlReaderTimer *m_readerTimer;
    RenderTimer *m_renderTimer;
    wxConfigBase *m_config;

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
    
    wxString          m_hostname;
    int               m_port;
    SOCKET            m_s;
    owwl_conn        *m_connection;

private:
    wxLongLong owwl_version_num(void)
    {
        return OWW_PROTO_VERSION;
    }

    DECLARE_DYNAMIC_CLASS(MyFrame)
    DECLARE_EVENT_TABLE()
};



class MyAuxilliaryFrame : public wxFrame
{
//DECLARE_CLASS(MyAuxilliaryFrame)
public:
    MyAuxilliaryFrame(wxFrame *parent, const wxString& desc)
    {
        Create(parent, desc); 
    }

private:
    wxGrid *m_grid;
    wxStatusBar * m_statusBar;
    bool Create(wxFrame *parent, const wxString& desc)
    {
        if ( !wxFrame::Create(parent, wxID_ANY,
                              wxString::Format(wxT("Image from %s"), desc),
                              wxDefaultPosition, wxDefaultSize,
                              wxDEFAULT_FRAME_STYLE | wxFULL_REPAINT_ON_RESIZE) )
            return false;

        m_grid = (wxGrid*)NULL;
        wxMenu *menu = new wxMenu;
        menu->Append(wxID_SAVE);

        wxMenuBar *mbar = new wxMenuBar;
        mbar->Append(menu, wxT("Something here?"));
        SetMenuBar(mbar);

        m_statusBar = CreateStatusBar(2);
        //SetStatusText("Loading", 1);

        UpdateStatusBar();
        m_grid = new wxGrid(this, wxID_ANY, wxPoint(0,0), wxDefaultSize);
        m_grid->EnableEditing(false);
        m_grid->EnableDragRowSize(false);
        m_grid->CreateGrid(0, 3);
        m_grid->SetLabelValue(wxHORIZONTAL, "Name", 0);
        m_grid->SetLabelValue(wxHORIZONTAL, "Value", 1);
        m_grid->SetLabelValue(wxHORIZONTAL, "Unit", 2);
        m_grid->AppendRows(12);
        m_grid->SetCellValue("Wind Speed", 0, 0);
        m_grid->SetCellValue("22.2", 0, 1);
        m_grid->SetCellValue("mph", 0, 2);
        m_grid->AutoSize();
//        m_grid->UpdateDimensions();
        SetClientSize(m_grid->GetSize());
//        SetSize(wxSize((2+m_grid->GetRows())*m_grid->GetRowSize(0), 
//                        m_grid->GetColLabelSize()+m_grid->GetColSize(0)
//                                 +m_grid->GetColSize(1)+m_grid->GetColSize(2)));
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
        //m_statusBar->SetStatusText("Running...", 1);
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

BEGIN_EVENT_TABLE(MyAuxilliaryFrame, wxFrame)
    EVT_ERASE_BACKGROUND(MyAuxilliaryFrame::OnEraseBackground)
    EVT_PAINT(MyAuxilliaryFrame::OnPaint)

    EVT_MENU(wxID_SAVE, MyAuxilliaryFrame::OnSave)
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

char g_tempStr[50];
char g_windStr[50];


static int
print_data(owwl_conn *conn, owwl_data *data, void *user_data)
{
  char linebuf[128], namebuff[128] ;
  int length ;

  if (data->str)
  {
    int arg = 0 ;

    while (arg >= 0)
    {
        int unit_class, unit = OwwlUnit_Metric ;

        unit_class = owwl_unit_class(data, arg) ;

        if ((unit_class >= 0) && (unit_class < OWWL_UNIT_CLASS_LIMIT)) 
            unit = 1; //unit_choices[unit_class] ;

        printf("In print_data %s %s %s %s\n", owwl_name(data, namebuff, 128, &length, 0),
        owwl_arg_stem((owwl_device_type_enum)data->device_type, data->device_subtype, arg),
        data->str(data, linebuf, 128, unit, -1, arg), 
        owwl_unit_name(data, unit, arg));

        if (OwwlDev_Temperature == data->device_type)
        {
            strcpy(g_tempStr, data->str(data, linebuf, 128, unit, -1, arg));
        }
        if (OwwlDev_Wind == data->device_type)
        {
            strcpy(g_windStr, data->str(data, linebuf, 128, unit, -1, arg));
        }
/*
        datalist_write(
            (client_conn_struct *)user_data,
            stream_data_arg_to_id(data, arg),
            owwl_name(data, namebuff, 128, &length, 0),
            owwl_arg_stem(data->device_type, data->device_subtype, arg),
            data->str(data, linebuf, 128, unit, -1, arg),
            owwl_unit_name(data, unit, arg)
            );
*/
        arg = owwl_next_arg(data, arg) ;
    }
    //printf("\n") ;
  }

  return 0 ;
}


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

    m_connection = NULL;
    m_canvas = NULL;
    m_statusbar = NULL;
    m_hostname = wxString(wxT("localhost"));
    m_port = 8899;
    m_s = -1;

    SetIcon(wxICON(oww));

    m_config = wxConfigBase::Get();
    m_config->Read("server", &m_hostname);
    m_port = m_config->ReadLong("port", m_port);

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
    //SetStatusText("Hi There!", 1);
    //wxLogStatus(this, wxT("pr %d"), 0);
    Refresh();

    SetTitle(wxString::Format(wxT("%s://%s:%d"), GetTitle(), m_hostname, (int)m_port));

    m_canvas = new MyCanvas( this, wxID_ANY, wxPoint(0,0), wxSize(474,441) );

    m_renderTimer = new RenderTimer(this);
    Show();
    m_renderTimer->start();

    m_readerTimer = new OwwlReaderTimer(this);
    m_readerTimer->start();

    {
        struct hostent  *host;
        struct sockaddr *address;
        struct sockaddr_in addr_in;
        host = gethostbyname(m_hostname.c_str()) ;
        if (host)
        {
            addr_in.sin_family = AF_INET;
            addr_in.sin_port   = htons(m_port);
            memcpy(&addr_in.sin_addr, host->h_addr_list[0], sizeof(addr_in.sin_addr));
            address = (struct sockaddr *) &addr_in;
            int addr_len = sizeof(addr_in);
            m_s = socket(address->sa_family, SOCK_STREAM, 0);
            if(m_s != -1)
            {
                m_connection = owwl_new_conn(m_s, NULL);
                if (connect(m_s, address, addr_len) == 0)
                {
                    int retval = owwl_read(m_connection);
                    switch(retval)
                    {
                        case Owwl_Read_Error:
                            SetStatusText("Protocol error");
                            break;
                        case Owwl_Read_Disconnect:
                            SetStatusText("Server disconnect");
                            break;
                        case Owwl_Read_Again:
                            SetStatusText("Read again");
                            break;
                        case Owwl_Read_Read_And_Decoded:
                            //SetStatusText("Read & Decode");
                            break;
                        default:
                            SetStatusText("Read default");
                            break;
                    }
                    owwl_foreach(m_connection, print_data, NULL/*client*/);
                }
                else
                {
                    wxLogStatus("Error: connect failed %d", errno);
                    close(m_s);
                }
            }
            else
            {
                wxLogStatus("Error: s<0 %d", errno);
            }
        }
        else
        {
          wxLogStatus(this, wxT("Unable to resolve host name %s"), m_hostname);
        }
    }

    return;
}

void MyFrame::OnQuit( wxCommandEvent &WXUNUSED(event) )
{
    owwl_free(m_connection);
    m_connection = NULL;
    close(m_s);
    m_renderTimer->Stop();
    delete m_renderTimer;
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
    array.Add("Version of owwl: " + owwl_version_num().ToString());

    (void)wxMessageBox( wxJoin(array, '\n'),
                        "One wire Weather",
                        wxICON_INFORMATION | wxOK );
}

class MyDevicesFrame : public wxFrame
{
public:
    MyDevicesFrame(wxWindow* parent, wxString title) :
        wxFrame(parent, wxID_ANY, title)
    {
    
        Connect(wxEVT_PAINT, wxPaintEventHandler(MyDevicesFrame::OnPaint));

        Show();
    }

private:
    wxGrid m_grid;
    void OnPaint(wxPaintEvent& WXUNUSED(event))
    {
        wxPaintDC dc(this);
        wxScopedPtr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));

        gc->SetFont(*wxNORMAL_FONT, *wxBLACK);
        gc->DrawText("Text Here", 0, 90/2);

        wxGraphicsFont gf = gc->CreateFont(wxNORMAL_FONT->GetPixelSize().y, "");
        gc->SetFont(gf);
        gc->DrawText("More Text", 0, (3*90)/2);
    }

    wxDECLARE_NO_COPY_CLASS(MyDevicesFrame);
};



BEGIN_EVENT_TABLE(MySetupDialog, wxDialog)
    EVT_BUTTON(wxID_ANY, MySetupDialog::OnButton)
END_EVENT_TABLE()



void MySetupDialog::OnButton (wxCommandEvent & event)
{
    event.Skip();
    return;
}


MySetupDialog::MySetupDialog ( wxWindow * parent, wxWindowID id, 
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

    SetEscapeId(wxID_CANCEL);

}

void MyFrame::OnSetup( wxCommandEvent &WXUNUSED(event) )
{
    wxString dialogText;
    MySetupDialog setupDialog(this, -1, _("Your very own dialog"),
                              wxPoint(100, 100), wxSize(200, 200));
    setupDialog.ShowModal();
}

void MyFrame::OnAuxilliary(wxCommandEvent &WXUNUSED(event))
{
    new MyAuxilliaryFrame(this, "Aux");
}

void MyFrame::OnMap( wxCommandEvent &WXUNUSED(event) )
{
    wxString url;
    m_config->Read("mapcmd", &url);
    (void)wxMessageBox( url,
                        "One wire Weather",
                        wxICON_INFORMATION | wxOK );

    char command[2048];
    sprintf(command, url, m_connection->latitude, m_connection->longitude);
    (void)wxMessageBox( wxString(command),
                        "One wire Weather",
                        wxICON_INFORMATION | wxOK );

    wxArrayString output;
    wxExecute(wxString(command), output);
}


void MyFrame::OnMessages( wxCommandEvent &WXUNUSED(event) )
{
    new MyDevicesFrame(this, "Messages");
}


void MyFrame::OnDevices(wxCommandEvent& WXUNUSED(event))
{
    new MyDevicesFrame(this, "Devices");
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


OwwlReaderTimer::OwwlReaderTimer(MyFrame* f) : wxTimer()
{
    OwwlReaderTimer::m_frame = f;
}
 
void OwwlReaderTimer::Notify()
{
#if 1
    int retval = owwl_read(m_frame->m_connection);
    switch(retval)
    {
        case Owwl_Read_Error:
            m_frame->SetStatusText("Protocol error");
            break;
        case Owwl_Read_Disconnect:
            m_frame->SetStatusText("Server disconnect");
            break;
        case Owwl_Read_Again:
            m_frame->SetStatusText("Read again");
            break;
        case Owwl_Read_Read_And_Decoded:
            //m_frame->SetStatusText("Read & Decoded");
            break;
        default:
            m_frame->SetStatusText("Read default");
            break;
    }
#endif

}

void OwwlReaderTimer::start()
{
    wxTimer::Start(10000);
}





RenderTimer::RenderTimer(MyFrame* f) : wxTimer()
{
    RenderTimer::m_frame = f;
}

void RenderTimer::Notify()
{
#if 0
    int retval = owwl_read(m_frame->m_connection);
    switch(retval)
    {
        case Owwl_Read_Error:
            m_frame->SetStatusText("Protocol error");
            break;
        case Owwl_Read_Disconnect:
            m_frame->SetStatusText("Server disconnect");
            break;
        case Owwl_Read_Again:
            m_frame->SetStatusText("Read again");
            break;
        case Owwl_Read_Read_And_Decoded:
            m_frame->SetStatusText("Read & Decoded");
            break;
        default:
            m_frame->SetStatusText("Read default");
            break;
    }
#endif
    m_frame->m_canvas->Refresh();
}

void RenderTimer::start()
{
    wxTimer::Start(100);
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
    m_frame = (MyFrame *)parent;
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
    owwl_data *od = NULL;
    float t = 0.0, s = 0.0, g = 0.0;
    float b = 0.0;

    if(m_frame)
    {
        if(m_frame->m_connection)
        {
           od = owwl_find(m_frame->m_connection, OwwlDev_Temperature, OwwlTemp_Thermometer, 0);
           t = od->device_data.temperature.T;
           //owwl_foreach(m_frame->m_connection, print_data, NULL/*client*/);
           od = owwl_find(m_frame->m_connection, OwwlDev_Wind, 0, 0);
           s = od->device_data.wind.speed;
           g = od->device_data.wind.gust;
           b = od->device_data.wind.bearing;
        }
    }
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
        dc.DrawBitmap( body_jpg, 0, top1_jpg.GetHeight());

    switch( lround(b/22.5) )
    {
        case 1:
        case 2:
            dc.DrawBitmap( bottom1_jpg, 0, top1_jpg.GetHeight() + body_jpg.GetHeight());
            break;
        case 3:
        case 4:
            dc.DrawBitmap( bottom2_jpg, 0, top1_jpg.GetHeight() + body_jpg.GetHeight());
            break;
        case 5:
        case 6:
            dc.DrawBitmap( bottom3_jpg, 0, top1_jpg.GetHeight() + body_jpg.GetHeight());
            break;
        case 7:
        case 8:
            dc.DrawBitmap( bottom4_jpg, 0, top1_jpg.GetHeight() + body_jpg.GetHeight());
            break;
        case 9:
        case 10:
            dc.DrawBitmap( bottom5_jpg, 0, top1_jpg.GetHeight() + body_jpg.GetHeight());
            break;
        case 11:
        case 12:
            dc.DrawBitmap( bottom6_jpg, 0, top1_jpg.GetHeight() + body_jpg.GetHeight());
            break;
        case 13:
        case 14:
            dc.DrawBitmap( bottom7_jpg, 0, top1_jpg.GetHeight() + body_jpg.GetHeight());
            break;
        case 15:
        case 16:
            dc.DrawBitmap( bottom8_jpg, 0, top1_jpg.GetHeight() + body_jpg.GetHeight());
            break;
        default:
            dc.DrawBitmap( bottom1_jpg, 0, top1_jpg.GetHeight() + body_jpg.GetHeight());
    }



/*    if (bottom1_jpg.IsOk())
        dc.DrawBitmap( bottom8_jpg, 0, 
            top1_jpg.GetHeight() + body_jpg.GetHeight());
*/
    if (rh_png.IsOk())
        dc.DrawBitmap( rh_png, 300, 180 );

    dc.SetTextForeground( wxT("WHITE") );
    wxString now = wxNow ();
    dc.DrawText (now, 100, 10);
    dc.DrawText( wxString::Format("%2.1f C", t), 30, 130 );

    dc.DrawText( wxString::Format("%2.1f m/s", s), 365, 20 );
    dc.DrawText( wxString::Format("%2.1f gusts", g), 365, 40 );
    dc.DrawText( wxString::Format("%2.1f bearing", b), 365, 60 );

    dc.SetBrush( wxBrush( wxT("white"), wxSOLID ) );
    dc.SetPen( *wxBLACK_PEN );
    dc.DrawCircle( 350, 100, 30);
#if 0
    if(m_frame)
    {
        if(m_frame->m_connection)
        {
            wxString str;
            str.Printf(wxT("%d"), m_frame->m_connection->data_count);
            m_frame->SetStatusText(str, 1); 
        }
    }
#endif
    //dc.SetBrush( *wxWHITE_BRUSH );
    //dc.SetPen( *wxRED_PEN );
    //dc.DrawRectangle( 170, 50, 60, 60 );

}


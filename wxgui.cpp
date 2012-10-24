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
#ifndef NOPCH
#include <wx/wxprec.h>
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

// wxWidget includes
#include <wx/log.h>
#include <wx/apptrait.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/wfstream.h>
#include <wx/quantize.h>
#include <wx/stopwatch.h>
#include <wx/graphics.h>
#include <wx/grid.h>
#include <wx/listctrl.h>
#include <wx/config.h>
#include <wx/aboutdlg.h>
#include <wx/utils.h> 
#include <wx/notifmsg.h>
#include <wx/propdlg.h>
#include <wx/spinctrl.h>
#include <wx/bookctrl.h>

// system includes
#ifndef __WXMSW__
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/time.h>
#define sock_error h_errno
#define sock_close close
#else
#include <windows.h>
#include <winsock.h>
//#include <Ws2tcpip.h>
#define lround(num) ( (long)(num > 0 ? num + 0.5 : ceil(num - 0.5)) )
#define sock_error WSAGetLastError()
#define sock_close _close
#endif

// app include
#include "wxgui.h"



#ifdef __cplusplus
extern "C" {
#endif
#define HAVE_GETTIMEOFDAY 1

#include "owwl.h"

/* Call a function for each data entry */
/* The function should return non-zero to break out of the loop */
int
owwl_foreach_jmc(owwl_conn *conn, owwl_func func, void *user_data)
{
    int i ;

    for (i=0; i<conn->data_count; ++i)
    {
        if (func(conn, &(conn->data[i]), user_data)) return -1 ;
    }

    return 0 ; /* Ok */
}

#ifdef __cplusplus
} /*extern "C"*/
#endif

enum
{
    ID_QUIT  = wxID_EXIT,
    ID_ABOUT = wxID_ABOUT,
    ID_MAIN_FRAME = 100,
    ID_AUX_FRAME,
    ID_AUXILLIARY = 200,
    ID_MAP,
    ID_TOGGLEUNITS,
    ID_MESSAGES,
    ID_SETUP,
    ID_DEVICES
};

class MyCanvas;
class MyAuxilliaryFrame;

wxString g_VersionStr = VERSIONSTR;
owwl_conn *g_connection;


#ifndef wxHAS_IMAGES_IN_RESOURCES
    #include "pixmaps/oww_xpm.xpm"
#endif



// ============================================================================
// declarations
// ============================================================================


// Custom application traits class which we use to override the default log
// target creation
class MyAppTraits : public wxGUIAppTraits
{
public:
    virtual wxLog *CreateLogTarget();
};



// ----------------------------------------------------------------------------
// custom log target
// ----------------------------------------------------------------------------

class MyLogGui : public wxLogGui
{
private:
    virtual void DoShowSingleLogMessage(const wxString& message,
                                        const wxString& title,
                                        int style)
    {
        wxMessageDialog dlg(NULL, message, title,
                            wxOK | wxCANCEL | wxCANCEL_DEFAULT | style);
        dlg.SetOKCancelLabels(wxID_COPY, wxID_OK);
        dlg.SetExtendedMessage("Note that this is a custom log dialog.");
        dlg.ShowModal();
    }
};

wxLog *MyAppTraits::CreateLogTarget()
{
    return new MyLogGui;
}



//-----------------------------------------------------------------------------
// MyApp
//-----------------------------------------------------------------------------

class MyApp: public wxApp
{
public:
    virtual bool OnInit();
    wxString GetCmdLine();
    void LogPlatform();

protected:
    virtual wxAppTraits *CreateTraits() { return new MyAppTraits; }
};


wxString default_unit_names[] = {"<Metric>", "<Imperial>", "<Alt1>", "<Alt2>"};
int unit_choices[OWWL_UNIT_CLASS_LIMIT];

//-----------------------------------------------------------------------------
// RenderTimer
//-----------------------------------------------------------------------------
class RenderTimer : public wxTimer
{
public:
    RenderTimer(MyCanvas* canvas);
    void Notify();
    void start();
private:
    MyCanvas * m_canvas;
    RenderTimer *m_renderTimer;
};



//-----------------------------------------------------------------------------
// MyCanvas
//-----------------------------------------------------------------------------
class MyCanvas: public wxPanel
{
public:
    MyCanvas( wxWindow *parent, wxWindowID, const wxPoint &pos, 
              const wxSize &size );
    ~MyCanvas();

    void OnPaint( wxPaintEvent &event );
    void CreateAntiAliasedBitmap();
    void DrawText(wxString str, wxColor fore, wxColor shadow, wxPoint pt );

    wxBitmap  body_jpg;
    wxBitmap  bottom1_jpg;
    wxBitmap  bottom2_jpg;
    wxBitmap  bottom3_jpg;
    wxBitmap  bottom4_jpg;
    wxBitmap  bottom5_jpg;
    wxBitmap  bottom6_jpg;
    wxBitmap  bottom7_jpg;
    wxBitmap  bottom8_jpg;
    wxBitmap  rh_png;
    wxBitmap  top1_jpg;
    wxBitmap  top2_jpg;
    wxBitmap  top3_jpg;

    MyFrame *m_frame;
    int xH, yH;
    RenderTimer *m_renderTimer;
    MyAuxilliaryFrame *m_auxilliaryFrame;

private:
    int m_counter;

    DECLARE_EVENT_TABLE()
};



//-----------------------------------------------------------------------------
// OwwlReaderTimer
//-----------------------------------------------------------------------------
class OwwlReaderTimer : public wxTimer
{
public:
    OwwlReaderTimer(MyFrame * canvas, unsigned int pollInt = 9);
    void Notify();
    void start();

private:
    unsigned int m_pollInterval;
    MyFrame * m_frame;
};



//-----------------------------------------------------------------------------
// MyAuxilliaryFrame
//-----------------------------------------------------------------------------
class MyAuxilliaryFrame : public wxDialog
{
public:
    MyAuxilliaryFrame(wxWindow *parent, MyCanvas * canvas, const wxString& desc)
    {
        m_grid = (wxGrid*)NULL;
        m_statusBar = (wxStatusBar*)NULL;
        Create(parent, canvas, desc); 
    } //MyAuxilliaryFrame c-tor
    ~MyAuxilliaryFrame();

    void SetGridUnits(void);

    wxGrid *m_grid;
    MyFrame *m_parentFrame;

private:
    bool m_updateGridUnits;
    MyCanvas *m_canvas;
    wxStatusBar * m_statusBar;
    enum gridColumns 
    {
        gridColName, 
        gridColData, 
        gridColValue, 
        gridColUnit
    };
    int InitPopulateCells(void);
    int PopulateCellVals(void);
    int UpdateCellsUnits(void);
    void OnPaint(wxPaintEvent&);

    bool Create(wxWindow *parent, MyCanvas * canvas, const wxString& desc)
    {
        if (!wxDialog::Create(parent, ID_AUX_FRAME, desc, 
                    wxDefaultPosition, wxDefaultSize,
                    wxDEFAULT_DIALOG_STYLE))
        {
            return false;
        }
        m_canvas = canvas;
        m_parentFrame = (MyFrame*)parent;

        m_grid = new wxGrid(this, wxID_ANY, wxPoint(0,0), wxDefaultSize);
        m_grid->EnableEditing(false);
        m_grid->EnableDragRowSize(false);
        m_grid->EnableDragColSize(false);
        m_grid->CreateGrid(0, 4);
        m_grid->SetLabelValue(wxHORIZONTAL, "  Name  ", gridColName);
        m_grid->SetLabelValue(wxHORIZONTAL, "  Data  ", gridColData);
        m_grid->SetLabelValue(wxHORIZONTAL, "  Value ", gridColValue);
        m_grid->SetLabelValue(wxHORIZONTAL, "  Unit  ", gridColUnit);
        InitPopulateCells();
        UpdateCellsUnits();
        PopulateCellVals();
        m_grid->SetRowLabelSize(wxGRID_AUTOSIZE);
        m_grid->SetColLabelSize(wxGRID_AUTOSIZE);
        m_grid->AutoSize();

        {
            wxPoint pt;
            wxSize gSz = m_grid->GetSize();
            pt.x = 0;
            pt.y += gSz.GetHeight();
            wxSize bSz = wxDefaultSize;
            bSz.SetWidth(gSz.GetWidth()-10);
            wxButton *b = new wxButton(this, wxID_OK, _("OK"), pt, bSz);
            bSz = b->GetDefaultSize();
#ifdef __WXOSX_COCOA__
            gSz.IncBy(0, bSz.GetHeight()*1.5);
#else
            gSz.IncBy(0, bSz.GetHeight());
#endif
            SetClientSize(gSz);
        }

        return true;
    } //Create

    DECLARE_EVENT_TABLE()
}; //class MyAuxilliaryFrame


void MyAuxilliaryFrame::SetGridUnits(void)
{
    m_updateGridUnits = true;
} //MyAuxilliaryFrame::SetGridUnits



void MyAuxilliaryFrame::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    if(true == m_updateGridUnits)
    {
        UpdateCellsUnits();
        m_updateGridUnits = false;
    }
    PopulateCellVals();
} //MyAuxilliaryFrame::OnPaint


MyAuxilliaryFrame::~MyAuxilliaryFrame()
{
    ;
} //MyAuxilliaryFrame d-tor



int MyAuxilliaryFrame::InitPopulateCells()
{
    if(NULL == m_grid) return 1;
    owwl_conn *conn = g_connection;
    owwl_data *data = NULL;
    if(NULL != conn)
    {
        if(NULL != conn->data)
        {
            data = conn->data;
            if (data->str)
            {
                int cntr = 0;
                int i;
                for (i=0; i<conn->data_count; ++i)
                {
                    char linebuf[128], namebuff[128];
                    int length;
                    int arg = 0;

                    while (arg >= 0)
                    {
                        int unit_class, unit;
                        linebuf[0] = '\0';
                        namebuff[0] = '\0';
                        unit_class = owwl_unit_class(data, arg) ;

                        if ((unit_class >= 0) && (unit_class < OWWL_UNIT_CLASS_LIMIT)) 
                            unit = unit_choices[unit_class] ;

                        m_grid->AppendRows();
                        m_grid->SetCellValue(owwl_name(&(data[i]), namebuff, 
                                                128, &length, 0), cntr, 0);
                        m_grid->SetCellValue(owwl_arg_stem(
                                        (owwl_device_type_enum) data[i].device_type, 
                                        data[i].device_subtype, arg),
                                cntr, 1);
                        cntr++;
                        arg = owwl_next_arg(&(data[i]), arg) ;
                    }
                }
            }
        }
    }
    return 0;
} //MyAuxilliaryFrame::InitPopulateCells


int MyAuxilliaryFrame::UpdateCellsUnits()
{
    if(NULL == m_grid) return 1;
    owwl_conn *conn = g_connection;
    owwl_data *data = NULL;
    if(NULL != conn)
    {
        if(NULL != conn->data)
        {
            data = conn->data;
            if (data->str)
            {
                int cntr = 0;
                int i;
                for (i=0; i<conn->data_count; ++i)
                {
                    int arg = 0;

                    while (arg >= 0)
                    {
                        int unit_class, unit;
                        unit_class = owwl_unit_class(data, arg) ;

                        if ((unit_class >= 0) && (unit_class < OWWL_UNIT_CLASS_LIMIT))
                            unit = unit_choices[unit_class];

                        m_grid->SetCellValue(owwl_unit_name(&(data[i]), unit, arg),
                                            cntr, 3);
                        cntr++;
                        arg = owwl_next_arg(&(data[i]), arg) ;
                    }
                }
            }
        }
    }
    return 0;
} //MyAuxilliaryFrame::UpdateCellsUnits

int MyAuxilliaryFrame::PopulateCellVals(void)
{
    if(NULL == m_grid) return 1;
    owwl_conn *conn = g_connection;
    owwl_data *data = NULL;
    if(NULL != conn)
    {
        if(NULL != conn->data)
        {
            data = conn->data;
            if (data->str)
            {
                char linebuf[128];
                int cntr = 0;
                int i;
                for (i=0; i<conn->data_count; ++i)
                {
                    int arg = 0;

                    while (arg >= 0)
                    {
                        linebuf[0] = '\0';
                        int unit_class, unit;
                        unit_class = owwl_unit_class(data, arg) ;

                        if ((unit_class >= 0) && (unit_class < OWWL_UNIT_CLASS_LIMIT)) 
                            unit = unit_choices[unit_class] ;

                        wxString old_val = m_grid->GetCellValue(cntr, 2);
                        wxString new_val = (data[i]).str(&(data[i]), linebuf, 128, unit, -1, arg);
                        if(false == old_val.IsSameAs(new_val))
                        {
                            m_grid->SetCellTextColour(cntr, 2, wxT("RED")); 
                        }
                        else
                        {
                            m_grid->SetCellTextColour(cntr, 2, m_grid->GetDefaultCellTextColour());
                        }
                        m_grid->SetCellValue(new_val, cntr, 2);
                        cntr++;
                        arg = owwl_next_arg(&(data[i]), arg) ;
                    }
                }
            }
        }
    }
#ifndef __WXOSX_COCOA__
    m_grid->AutoSize();
#endif
    return 0;
} //MyAuxilliaryFrame::PopulateCellVals


// ----------------------------------------------------------------------------
// MySettingsDialogy
// ----------------------------------------------------------------------------
class MySettingsDialog: public wxPropertySheetDialog
{
DECLARE_CLASS(MySettingsDialog)
public:
    MySettingsDialog(wxWindow* parent);
    ~MySettingsDialog();

    wxPanel* CreateServerSettingsPage(wxWindow* parent);
    wxPanel* CreateDisplaySettingsPage(wxWindow* parent);
    wxPanel* CreateLaunchSettingsPage(wxWindow* parent);

    wxTextCtrl        *serverText;
    wxSpinCtrl        *portSpin;
    wxSpinCtrl        *pollSpin;
    wxCheckBox        *launchAtStart;
    wxChoice          *unitsChoice;
    wxCheckBox        *animateDisplay;
    wxCheckBox        *restoreAuxFrame;
    wxChoice          *browserChoice;
    wxTextCtrl        *urlCmdText;

protected:
    enum {
        ID_SERVER_TEXT= 200,
        ID_PORT_SPIN,
        ID_POLL_SPIN,
        ID_LAUNCH_CHECK,
        ID_UNITS_CHOICE,
        ID_ANIMATE_CHECK,
        ID_RESTOREAUX_CHECK,
        ID_BROWSER_CHOICE,
        ID_URLCMD_TEXT
    };

DECLARE_EVENT_TABLE()
}; //class MySettingsDialog


// ----------------------------------------------------------------------------
// MyFrame
// ----------------------------------------------------------------------------
class MyFrame: public wxFrame
{
    OwwlReaderTimer *m_readerTimer;
    wxConfigBase *m_config;

public:
    MyFrame();
    ~MyFrame();

    void OnAbout( wxCommandEvent &event );
    void OnAuxilliary( wxCommandEvent &event );
    void OnMap( wxCommandEvent &event );
    void OnMessages( wxCommandEvent &event );
#ifdef __WXMOTIF__
    void OnMenuToggleUnits( wxCommandEvent &event );
#endif
#if 0
    void OnSetup( wxCommandEvent &event );
    void OnDevices( wxCommandEvent &event );
#endif
    void OnQuit( wxCommandEvent &event );

    owwl_buffer buff;
    
    wxLogWindow       *m_logWindow;
    wxMenu            *menuImage;
    wxMenu            *subMenu;
    MyCanvas          *m_canvas;
    wxStatusBar       *m_statusbar;
    wxString           m_hostname;
    int                m_port;
    unsigned int       m_pollInterval;
    SOCKET             m_s;
    owwl_conn         *m_connection;
    wxString           m_mapurl;
    bool               m_launchAtStart;
    int                m_units;
    bool               m_animateDisplay;
    int                m_browser;
    bool               m_restoreAuxFrame;
private:
    enum
    {
        Menu_SubMenu = 450,
        Menu_SubMenu_Radio0,
        Menu_SubMenu_Radio1,
        Menu_SubMenu_Radio2,
        Menu_SubMenu_Radio3,
        DIALOGS_PROPERTY_SHEET,
    };

    void OnMenuSetUnits(wxCommandEvent &event);
    int InitServerConnection(void);
    void OnPropertySheet(wxCommandEvent& event);

    wxLongLong owwl_version_num(void)
    {
        return OWW_PROTO_VERSION;
    };

    void changeUnits(int units);

    DECLARE_DYNAMIC_CLASS(MyFrame)
    DECLARE_EVENT_TABLE()
}; //class MyFrame



// ============================================================================
// implementations
// ============================================================================

//-----------------------------------------------------------------------------
// MyAuxilliaryFrame
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(MyAuxilliaryFrame, wxDialog)
    EVT_PAINT(MyAuxilliaryFrame::OnPaint)
#if 0
    EVT_ERASE_BACKGROUND(MyAuxilliaryFrame::OnEraseBackground)
    EVT_MENU(wxID_SAVE, MyAuxilliaryFrame::OnSave)
#endif
END_EVENT_TABLE()

//-----------------------------------------------------------------------------
// MyFrame
//-----------------------------------------------------------------------------

static int
print_data(owwl_conn * /*conn*/, owwl_data *data, void * /*user_data*/)
{
#if 0
  char linebuf[128], namebuff[128] ;
  int length ;
#endif
  if (data->str)
  {
    int arg = 0 ;

    while (arg >= 0)
    {
        int unit_class, unit = OwwlUnit_Metric ;

        unit_class = owwl_unit_class(data, arg) ;

        if ((unit_class >= 0) && (unit_class < OWWL_UNIT_CLASS_LIMIT)) 
            unit = unit_choices[unit_class] ;
#if 0
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
#endif
        arg = owwl_next_arg(data, arg) ;
    }
  }

  return 0 ;
}

void MyFrame::changeUnits(int units)
{
    int i;
    for (i=0; i<OWWL_UNIT_CLASS_LIMIT; ++i)
    {
        unit_choices[i] = units;
    }

    if(NULL != subMenu)
    {
        subMenu->Check(Menu_SubMenu_Radio0, (m_units==OwwlUnit_Metric)?true:false);
        subMenu->Check(Menu_SubMenu_Radio1, (m_units==OwwlUnit_Imperial)?true:false);
        subMenu->Check(Menu_SubMenu_Radio2, (m_units==OwwlUnit_Alt1)?true:false);
        subMenu->Check(Menu_SubMenu_Radio3, (m_units==OwwlUnit_Alt2)?true:false);
    }

    return;
}


IMPLEMENT_DYNAMIC_CLASS( MyFrame, wxFrame )
BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(ID_ABOUT, MyFrame::OnAbout)
    EVT_MENU(ID_QUIT,  MyFrame::OnQuit)
    EVT_MENU(ID_AUXILLIARY, MyFrame::OnAuxilliary)
    EVT_MENU(ID_MAP,   MyFrame::OnMap)
#ifdef __WXMOTIF__
    EVT_MENU(ID_TOGGLEUNITS,  MyFrame::OnMenuToggleUnits)
#endif
    EVT_MENU(ID_MESSAGES,  MyFrame::OnMessages)
    EVT_MENU(Menu_SubMenu_Radio0, MyFrame::OnMenuSetUnits)
    EVT_MENU(Menu_SubMenu_Radio1, MyFrame::OnMenuSetUnits)
    EVT_MENU(Menu_SubMenu_Radio2, MyFrame::OnMenuSetUnits)
    EVT_MENU(Menu_SubMenu_Radio3, MyFrame::OnMenuSetUnits)
    EVT_MENU(DIALOGS_PROPERTY_SHEET, MyFrame::OnPropertySheet)
END_EVENT_TABLE()


MyFrame::~MyFrame()
{
    {
#ifdef __WXMSW__
        WSACleanup();
#endif
    };
    // save the frame position
    int x, y, w, h;
    GetClientSize(&w, &h);
    GetPosition(&x, &y);
    m_config->Write(_T("/MainFrame/x"), (long) x);
    m_config->Write(_T("/MainFrame/y"), (long) y);
    m_config->Write(_T("/MainFrame/w"), (long) w);
    m_config->Write(_T("/MainFrame/h"), (long) h);
} //MyFrame d-tor



MyFrame::MyFrame()
    : wxFrame( (wxFrame *)NULL, ID_MAIN_FRAME, wxT("oww-wxgui"), 
                wxPoint(20, 20), 
                wxSize(474, 441),
                wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER|wxMAXIMIZE_BOX)
              )
{
    menuImage = NULL;
    subMenu = NULL;
    m_connection = NULL;
    m_canvas = NULL;
    m_statusbar = NULL;
    m_hostname = wxString(wxT("localhost"));
    m_port = 8899;
    m_pollInterval = 10;
    m_s = -1;
    m_units = OwwlUnit_Imperial;
#if 0    
    buff = Owwl_Buffer_Init;
#else
    memset(&buff, NULL, sizeof(owwl_buffer));
#endif
    m_browser = 0;
    m_mapurl = wxEmptyString;
    m_restoreAuxFrame = false;
#ifdef __WXMSW__
    WORD wVersionRequested;
    WSADATA wsaData;
    int wsaerr;

    // Using MAKEWORD macro, Winsock version request 2.2
    wVersionRequested = MAKEWORD(2, 2);
    wsaerr = WSAStartup(wVersionRequested, &wsaData);
    if (wsaerr != 0)
    {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.*/
        (void)wxLogVerbose(_("The Winsock dll not found!"),
                               "One wire Weather", wxICON_INFORMATION | wxOK );
    }
    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2 )
    {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.*/
        (void)wxLogVerbose(wxString::Format("Winsock dll is version %u.%u",
                                  LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion)),
                                  "One wire Weather", wxICON_INFORMATION | wxOK );
        WSACleanup();
    }
#endif

    SetIcon(wxICON(oww));

    m_config = wxConfigBase::Get();
    m_config->Read("server", &m_hostname);
    m_port = m_config->ReadLong("port", m_port);
    m_pollInterval = m_config->ReadLong("pollInterval", m_pollInterval);
    m_units = m_config->Read("units", OwwlUnit_Imperial);
    changeUnits(m_units);
    m_browser = m_config->Read("browser", (long int)0);
    m_config->Read("mapurl", &m_mapurl);
    m_config->Read("launchStStart", &m_launchAtStart);
    m_config->Read("animateDisplay", &m_animateDisplay);

    wxMenuBar *menu_bar = new wxMenuBar();
    menuImage = new wxMenu;
    menuImage->Append(ID_AUXILLIARY, wxT("Auxilary"), "See other device values");
#ifdef __WXMOTIF__
    // Motif doesn't do submenus, soooo....
    menuImage->Append(ID_TOGGLEMENU, wxT("Toggle Units"), "Swap Meteric and Imperial");
#else
    subMenu = new wxMenu;
    subMenu->AppendRadioItem(Menu_SubMenu_Radio0, wxT("Metric"), wxT("Metric"));
    subMenu->AppendRadioItem(Menu_SubMenu_Radio1, wxT("Imperical"), wxT("Imperical"));
    subMenu->AppendRadioItem(Menu_SubMenu_Radio2, wxT("Alt 1"), wxT("Alt 1"));
    subMenu->AppendRadioItem(Menu_SubMenu_Radio3, wxT("Alt 2"), wxT("Alt 2"));
    subMenu->Check(Menu_SubMenu_Radio0, (m_units==OwwlUnit_Metric));
    subMenu->Check(Menu_SubMenu_Radio1, (m_units==OwwlUnit_Imperial));
    subMenu->Check(Menu_SubMenu_Radio2, (m_units==OwwlUnit_Alt1));
    subMenu->Check(Menu_SubMenu_Radio3, (m_units==OwwlUnit_Alt2));

    menuImage->Append(Menu_SubMenu, wxT("Change Units"), subMenu);
#endif
    menuImage->Append(ID_MAP, wxT("Map"), "Map this station");
    menuImage->Append(ID_MESSAGES, wxT("Messages"), "Show Messages");
    menuImage->AppendSeparator();
    menuImage->Append(DIALOGS_PROPERTY_SHEET, wxT("Setup"), "Edit Preferences");
#ifndef __WXOSX_COCOA__
    menuImage->AppendSeparator();
#endif
    menuImage->Append(ID_ABOUT, wxT("&About"));
    menuImage->Append(ID_ABOUT, wxT("&Help"));
    menuImage->AppendSeparator();
    menuImage->Append(ID_QUIT, wxT("E&xit\tCtrl-Q"));
    menu_bar->Append(menuImage, wxT("Menu"));
    SetMenuBar(menu_bar);

    m_statusbar = CreateStatusBar(2);
    int widths[] = { -1, 200 };
    SetStatusWidths( 2, widths );

    m_config->SetPath(_T("/MainFrame"));
    // restore frame position and size
    int x = m_config->Read(_T("x"), 50);
    int y = m_config->Read(_T("y"), 50);
    int w, h;
    GetClientSize(&w, &h);
    w = m_config->Read(_T("w"), w);
    h = m_config->Read(_T("h"), h);
    if(0 > w || 0 > h)
    {
        w = h = 10;
    }
    Move(x, y);

    m_canvas = new MyCanvas( this, wxID_ANY, wxPoint(0,0), wxSize(474,441) );
    Show();

    m_canvas->m_renderTimer = new RenderTimer(m_canvas);
    m_canvas->m_renderTimer->start();

    m_readerTimer = new OwwlReaderTimer(this);
    m_readerTimer->start();

    if(0 == InitServerConnection())
    {
        SetTitle(wxString::Format(wxT("%s://%s:%d"), GetTitle(), m_hostname, (int)m_port));
    }
    
    if(true == m_restoreAuxFrame)
    {
        m_canvas->m_auxilliaryFrame = new MyAuxilliaryFrame(this, m_canvas, "Auxilliary Data");
        m_canvas->m_auxilliaryFrame->Show();
    }
    return;
} //MyFrame c-tor


int MyFrame::InitServerConnection(void)
{
    int retval = 0;
    {
        struct hostent  *host;
        struct sockaddr *address;
        struct sockaddr_in addr_in;

        memset(&addr_in, sizeof(struct sockaddr_in), 0);
        bool ipnumaddr = m_hostname.Matches("??.??.??.??");
        if(false == ipnumaddr)
        {
            host = gethostbyname(m_hostname.c_str()) ;
            if(NULL == host) 
            {
                wxLogVerbose(wxString::Format("gethostbyname=%d", sock_error),
                                   "One wire Weather", wxICON_INFORMATION | wxOK );
            }
        }
        else
        {
            addr_in.sin_addr.s_addr = inet_addr(m_hostname.c_str());
            host = gethostbyaddr((const char *)&addr_in, sizeof(struct sockaddr_in), AF_INET);
            if(NULL == host) 
            {
                wxLogVerbose(wxString::Format("gethostbyaddr=%d", sock_error),
                                   "One wire Weather", wxICON_INFORMATION | wxOK );
            }
        }
        if (NULL != host)
        {
            addr_in.sin_family = AF_INET;
            addr_in.sin_port   = htons(m_port);
            memcpy(&addr_in.sin_addr, host->h_addr_list[0], sizeof(addr_in.sin_addr));
            address = (struct sockaddr *) &addr_in;
            int addr_len = sizeof(addr_in);

            m_s = socket(address->sa_family, SOCK_STREAM, 0);
            if(m_s != -1)
            {
                g_connection = m_connection = owwl_new_conn(m_s, NULL);
                if (connect(m_s, address, addr_len) == 0)
                {
                    int retval = owwl_read(m_connection);
                    switch(retval)
                    {
                        case Owwl_Read_Error:
                            wxLogVerbose("Protocol error");
                            retval = -1;
                            // drop thru to disconnect break;
                        case Owwl_Read_Disconnect:
                            wxLogVerbose("Server disconnect");
                            sock_close(m_s);
                            m_s = -1;
                            g_connection = m_connection = NULL;
                            retval = -1;
                            break;
                        case Owwl_Read_Again:
                            wxLogVerbose("Read again");
                            owwl_tx_poll_servers(m_connection);
                            if(-1==retval)wxLogVerbose("owwl_tx_poll_servers failed");
                            retval = -1;
                            break;
                        case Owwl_Read_Read_And_Decoded:
                            wxLogVerbose("Read & Decode");
                            retval = owwl_tx_build(m_connection, OWW_TRX_MSG_WSDATA, &(buff));
                            if(-1==retval)wxLogVerbose("owwl_tx_build failed");
                            retval = owwl_tx(m_connection, &(buff));
                            if(-1==retval)wxLogVerbose("owwl_tx failed");
                            break;
                        default:
                            retval = -1;
                            wxLogVerbose("Read default");
                            break;
                    }
                    if(m_connection)
                    {
                        //owwl_foreach(m_connection, print_data, NULL);
                    }
                }
                else
                {
                    wxLogVerbose("Error: connect failed %d", errno);
                    owwl_free(m_connection);
                    sock_close(m_s);
                    m_s = -1;
                    g_connection = m_connection = NULL;
                    retval = -1;
                } //connect()
            }
            else
            {
                wxLogVerbose("Error: s<0 %d", errno);
                retval = -1;
            } //socket()
        }
        else
        {
            wxLogVerbose(wxT("Unable to resolve host name %s"), m_hostname);
            retval = -1;
        } //NULL!=host
    }
    return retval;
} //MyFrame::InitServerConnection



void MyFrame::OnMenuSetUnits(wxCommandEvent& event)
{
    m_units = event.GetId() - Menu_SubMenu_Radio0;

    if(NULL != m_connection)
    {
        changeUnits(m_units);
    }
    if(NULL != m_canvas->m_auxilliaryFrame)
    {
        m_canvas->m_auxilliaryFrame->SetGridUnits();
    }
    return;
} //MyFrame::OnMenuSetUnits


void MyFrame::OnQuit( wxCommandEvent &WXUNUSED(event) )
{
    m_readerTimer->Stop();
    delete m_readerTimer;

    owwl_free(m_connection);
    sock_close(m_s);
    m_s = -1;
    g_connection = m_connection = NULL;
    
    Close( true );
} //MyFrame::OnQuit


void MyFrame::OnAbout( wxCommandEvent &WXUNUSED(event) )
{
#if 0
    wxArrayString array;

    array.Add("Oww");
    array.Add("One wire weather");
    array.Add("(c) Dr. Simon J. Melhuish");

    array.Add(wxEmptyString);
    array.Add("Version: " + g_VersionStr);
    array.Add("Version of owwl: " + owwl_version_num().ToString());
        m_hyperlink = new wxGenericHyperlinkCtrl(this,
                                          wxID_ANY,
                                          wxT("Oww website"),
                                          wxT("oww.sourceforge.net"));
        m_hyperlink = new wxGenericHyperlinkCtrl(this,
                                          wxID_ANY,
                                          wxT("wxWidgets website"),
                                          wxT("www.wxwidgets.org"));

    (void)wxMessageBox( wxJoin(array, '\n'),
                        "One wire Weather",
                        wxICON_INFORMATION | wxOK );
#else
    wxAboutDialogInfo info;
    //info.SetIcon();
    info.SetName(_("oww-wxgui"));
    info.SetDescription(_("One wire Weather GUI"));
    info.AddDeveloper("Mark Clayton");
    info.AddDeveloper("\nDr. Simon Melhuish, author of oww - Thank You!");
    info.AddDeveloper("\nThe wxWidgets team! - Thank You!");
    info.SetWebSite(_("www.mark-clayton.com/oww-wxgui"), _("oww-wxgui website"));
    info.SetVersion(g_VersionStr);
    info.SetCopyright(_T("(C) 2012 Mark Clayton <mark_clayton@users.sourceforge.net>"));

    wxAboutBox(info);
    
#endif
} //MyFrame::OnAbout

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
    wxGrid *m_grid;
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
}; //class MyDeviceFrame



void MyFrame::OnAuxilliary(wxCommandEvent &WXUNUSED(event))
{
    m_restoreAuxFrame = true;
    m_canvas->m_auxilliaryFrame = new MyAuxilliaryFrame(this, m_canvas, "Auxilliary Data");
    m_canvas->m_auxilliaryFrame->Show();
} //MyFrame::OnAuxilliary



void MyFrame::OnMessages( wxCommandEvent &WXUNUSED(event) )
{
    m_logWindow->Show();
} //MyFrame::OnMessages



void MyFrame::OnMap( wxCommandEvent &WXUNUSED(event) )
{
    if(NULL != m_connection)
    {
        wxLogVerbose("OnMap Latitude=%3.4f", m_connection->latitude);
        wxLogVerbose("OnMap Longitude=%3.4f", m_connection->longitude);

        char command[2048];
        sprintf(command, "open /Applications/Safari.app/Contents/MacOS/Safari " + 
                                 m_mapurl, m_connection->latitude, m_connection->longitude);
        wxArrayString output;
        wxExecute(wxString(command), output);
    }
    else
    {
        wxLogVerbose("Can not map a server location if you're not connected to a server");
    }
} //MyFrame::OnMap


#ifdef __WXMOTIF__
void MyFrame::OnMenuToggleUnits( wxCommandEvent &WXUNUSED(event) )
{
    if(NULL != m_connection)
    {
        changeUnits((unit_choices[0]==OwwlUnit_Imperial) ? 
                                          OwwlUnit_Metric : OwwlUnit_Imperial);
    }
    if(NULL != m_canvas->m_auxilliaryFrame)
    {
        m_canvas->m_auxilliaryFrame->SetGridUnits();
    }
} //MyFrame::OnMenuToggleUnits
#endif

void MyFrame::OnPropertySheet(wxCommandEvent& WXUNUSED(event))
{
    MySettingsDialog dialog(this);

    dialog.serverText->SetValue(m_hostname);
    dialog.portSpin->SetValue(m_port);
    dialog.pollSpin->SetValue(m_pollInterval);
    dialog.launchAtStart->SetValue(true);
    dialog.unitsChoice->SetSelection(unit_choices[0]);
    dialog.animateDisplay->SetValue(true);
    dialog.restoreAuxFrame->SetValue(true);
    dialog.browserChoice->SetSelection(m_browser);
    dialog.urlCmdText->SetValue(m_mapurl);

    switch(dialog.ShowModal())
    {
        case wxID_CANCEL:
            break;
        case wxID_OK:
            m_hostname = dialog.serverText->GetValue();
            m_port = dialog.portSpin->GetValue();
            m_pollInterval = dialog.pollSpin->GetValue();
            m_launchAtStart = dialog.launchAtStart->GetValue();
            m_units = dialog.unitsChoice->GetSelection();
            m_animateDisplay = dialog.animateDisplay->GetValue();
            m_restoreAuxFrame = dialog.restoreAuxFrame->GetValue();
            m_browser = dialog.browserChoice->GetSelection();
            m_mapurl = dialog.urlCmdText->GetValue();
            m_config = wxConfigBase::Get();
            m_config->Write("server", m_hostname);
            m_config->Write("port", m_port);
            m_config->Write("pollInterval", m_pollInterval);
            m_config->Write("launchStStart", m_launchAtStart);
            m_config->Write("units", m_units);
            changeUnits(m_units);
            m_config->Write("animateDisplay", m_animateDisplay);
            m_config->Write("restoreAuxFrame", m_restoreAuxFrame);
            m_config->Write("browser", m_browser);
            m_config->Write("mapurl", m_mapurl);
            break;
        default:
            wxASSERT(true);
    }
} //MyFrame::OnPropertySheet


// ----------------------------------------------------------------------------
// SettingsDialog
// ----------------------------------------------------------------------------

IMPLEMENT_CLASS(MySettingsDialog, wxPropertySheetDialog)

BEGIN_EVENT_TABLE(MySettingsDialog, wxPropertySheetDialog)

END_EVENT_TABLE()

MySettingsDialog::MySettingsDialog(wxWindow* win)
{
    SetExtraStyle(wxDIALOG_EX_CONTEXTHELP|wxWS_EX_VALIDATE_RECURSIVELY);

    Create(win, wxID_ANY, _("Preferences"), wxDefaultPosition, wxSize(500,300), 
                                  wxDEFAULT_DIALOG_STYLE);

    // If using a toolbook, also follow Mac style and don't create buttons
    CreateButtons(wxOK | wxCANCEL | wxHELP);

    wxBookCtrlBase* notebook = GetBookCtrl();

    wxPanel* serverSettings ;
    wxPanel* displaySettings;
    wxPanel* launchSettings ;

    serverSettings = CreateServerSettingsPage(notebook);
    displaySettings = CreateDisplaySettingsPage(notebook);
    launchSettings = CreateLaunchSettingsPage(notebook);

    notebook->AddPage(serverSettings, _("Server"), true);
    notebook->AddPage(displaySettings, _("Display"), false);
    notebook->AddPage(launchSettings, _("URL Launch"), false);

    LayoutDialog();
} //MySettingsDialog c-tor

MySettingsDialog::~MySettingsDialog()
{
} //MySettingsDialog d-tor

wxPanel* MySettingsDialog::CreateServerSettingsPage(wxWindow* parent)
{
    wxPanel* panel = new wxPanel(parent, wxID_ANY);

    wxBoxSizer *topSizer = new wxBoxSizer( wxVERTICAL );

    // Connect to server on startup
    wxBoxSizer* itemSizer = new wxBoxSizer( wxVERTICAL );
    serverText = new wxTextCtrl(panel, ID_SERVER_TEXT, wxEmptyString, wxDefaultPosition, 
                                wxSize(300, -1), wxTE_LEFT);
    serverText->SetFocus();
    portSpin = new wxSpinCtrl(panel, ID_PORT_SPIN, wxEmptyString, wxDefaultPosition, 
                                wxSize(100, -1), wxSP_ARROW_KEYS, 7000, 65500, 8899);
    pollSpin = new wxSpinCtrl(panel, ID_POLL_SPIN, wxEmptyString, wxDefaultPosition,
                                wxSize(50, -1), wxSP_ARROW_KEYS, 1, 65, 5);
    launchAtStart = new wxCheckBox(panel, ID_LAUNCH_CHECK, _("Connect on startup"), 
                                wxDefaultPosition, wxDefaultSize);
    itemSizer->Add(new wxStaticText(panel, wxID_STATIC, _("Server:")), 0,
                                wxALIGN_CENTER_VERTICAL, 5);
    itemSizer->Add(serverText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 2);
    itemSizer->Add(new wxStaticText(panel, wxID_STATIC, _("Port : ")), 0, 
                                wxALIGN_CENTER_VERTICAL, 5);
    itemSizer->Add(portSpin, 0, wxALL|wxALIGN_CENTER_VERTICAL, 2);
    itemSizer->Add(new wxStaticText(panel, wxID_STATIC, _("Poll Interval (seconds) : ")), 0, 
                                wxALIGN_CENTER_VERTICAL, 5);
    itemSizer->Add(pollSpin, 0, wxALL|wxALIGN_CENTER_VERTICAL, 2);
    itemSizer->Add(launchAtStart, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    topSizer->Add(itemSizer, 1, wxGROW|wxALIGN_CENTRE|wxALL, 5 );
    panel->SetSizerAndFit(topSizer);

    return panel;
} // MySettingsDialog::CreateServerSettingsPage

wxPanel* MySettingsDialog::CreateDisplaySettingsPage(wxWindow* parent)
{
    wxPanel* panel = new wxPanel(parent, wxID_ANY);

    wxBoxSizer *topSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer *itemSizer = new wxBoxSizer( wxVERTICAL );

    itemSizer->Add(new wxStaticText(panel, wxID_ANY, _("Units:")), 0, wxALIGN_CENTER_VERTICAL, 5);
    // Display Units: Metric, Imperial, Alt 1, Alt 2
    wxArrayString unitChoices;
    unitChoices.Add(wxT("Metric"));
    unitChoices.Add(wxT("Imperial"));
    unitChoices.Add(wxT("Alt 1"));
    unitChoices.Add(wxT("Alt 2"));
    unitsChoice = new wxChoice(panel, ID_UNITS_CHOICE, wxDefaultPosition, 
                               wxDefaultSize, unitChoices);
    unitsChoice->SetSelection(1);
    unitsChoice->SetFocus();
    animateDisplay = new wxCheckBox(panel, ID_ANIMATE_CHECK, _("Animate"), 
                               wxDefaultPosition, wxDefaultSize);
    restoreAuxFrame = new wxCheckBox(panel, ID_RESTOREAUX_CHECK, _("Restore Auxilliary Window"), 
                               wxDefaultPosition, wxDefaultSize);
    itemSizer->Add(unitsChoice, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    itemSizer->Add(animateDisplay, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    itemSizer->Add(restoreAuxFrame, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    topSizer->Add(itemSizer, 0, wxGROW|wxALL, 5);
    panel->SetSizerAndFit(topSizer);

    return panel;
} //MySettingsDialog::CreateDisplaySettingsPage



wxPanel* MySettingsDialog::CreateLaunchSettingsPage(wxWindow* parent)
{
    wxPanel* panel = new wxPanel(parent, wxID_ANY);

    wxBoxSizer *topSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer *item0 = new wxBoxSizer( wxVERTICAL );

    wxStaticBox* staticBox3 = new wxStaticBox(panel, wxID_ANY, _("Browser Options:"));
    wxBoxSizer* styleSizer = new wxStaticBoxSizer( staticBox3, wxVERTICAL );
    item0->Add(styleSizer, 1, wxGROW|wxALL, 1);
    wxBoxSizer* itemSizer2 = new wxBoxSizer( wxVERTICAL );
    // Browser options
    wxArrayString browserChoices;
    browserChoices.Add(wxT("Firefox"));
    browserChoices.Add(wxT("Safari"));
    browserChoices.Add(wxT("Opera"));
    browserChoices.Add(wxT("Camino"));
    browserChoices.Add(wxT("Inernet Explorer"));
    browserChoices.Add(wxT("Chrome"));
    browserChoices.Add(wxT("Lynx"));
    browserChoices.Add(wxT("Netscape"));
    browserChoice = new wxChoice(panel, ID_BROWSER_CHOICE, wxDefaultPosition, 
                                 wxDefaultSize, browserChoices);
    browserChoice->SetSelection(1);
    browserChoice->SetFocus();
    urlCmdText = new wxTextCtrl(panel, ID_URLCMD_TEXT, wxEmptyString, wxDefaultPosition, 
                                wxSize(400, -1), wxTE_LEFT);

    itemSizer2->Add(new wxStaticText(panel, wxID_ANY, _("Browser:")), 0, 
                                wxALIGN_CENTER_VERTICAL, 2);
    itemSizer2->Add(browserChoice, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    itemSizer2->Add(new wxStaticText(panel, wxID_ANY, _("Map URL:")), 0, 
                                wxALIGN_CENTER_VERTICAL, 2);
    itemSizer2->Add(urlCmdText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 2);
    styleSizer->Add(itemSizer2, 0, wxGROW|wxALL, 2);

    topSizer->Add( item0, 1, wxGROW|wxALIGN_CENTRE|wxALL, 5 );

    panel->SetSizerAndFit(topSizer);

    return panel;
} //MySettingsDialog::CreateLaunchSettingsPage



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

#ifdef WXOWWGUI_LOG_TO_FILE
    wxFileName logPath = wxFileName(wxT("UpdaterLog.txt"));
    if( logPath.Normalize(wxPATH_NORM_ALL, muApp::GetAppDir().GetPath()) )
    {
        // create log
        logFile.Open(logPath.GetFullPath(), wxT("w"));
        logTarget = new wxLogStderr(logFile.fp()); 
        logTargetOld = wxLog::GetActiveTarget();
        logTarget->SetVerbose(TRUE); 
        wxLog::SetActiveTarget(logTarget); 
        }
#else
    wxLogWindow *m_logWindow = new wxLogWindow(frame, wxT("Log") );
    wxFrame *pLogFrame = m_logWindow->GetFrame();
    pLogFrame->SetWindowStyle(wxDEFAULT_FRAME_STYLE|wxSTAY_ON_TOP);
    pLogFrame->SetSize( wxRect(0,50,400,250) );
    m_logWindow->SetVerbose(TRUE);
    wxLog::SetActiveTarget(m_logWindow);
    m_logWindow->Show();
#endif

    wxLogVerbose("Welcome to oww-wxgui");

    LogPlatform();

    wxLogVerbose("cmdln: %s", MyApp::GetCmdLine().c_str());

    wxLogVerbose("latitude==%f", ((MyFrame*)frame)->m_connection->latitude);
    wxLogVerbose("longitude==%f", ((MyFrame*)frame)->m_connection->longitude);

    frame->Show( true );

    return true;
} //MyApp:OnInit


wxString MyApp::GetCmdLine()
{
    wxString s;
    for(int i=1; i<argc; ++i)
    {
        s.Append(argv[i]);
        s.Append(wxT(" "));
    }
    return s;
} // MyApp::GetCmdLine


// ----------------------------------------------------------------------------
//  debug
// ----------------------------------------------------------------------------
void MyApp::LogPlatform()
{
    wxPlatformInfo p = wxPlatformInfo::Get();
    wxLogVerbose(wxT("Platform details:"));
    wxLogVerbose(wxT(" * CPU Count: %d"), wxThread::GetCPUCount());
    wxLogVerbose(wxT(" * OS: %s"), wxGetOsDescription().c_str());
    wxLogVerbose(wxT(" * OS ID: %s"), p.GetOperatingSystemIdName().c_str());
    wxLogVerbose(wxT(" * OS Family: %s"), p.GetOperatingSystemFamilyName().c_str());
    wxLogVerbose(wxT(" * OS Version: %d.%d"), p.GetOSMajorVersion(), p.GetOSMinorVersion());
    wxLogVerbose(wxT(" * Toolkit Version: %d.%d"), p.GetToolkitMajorVersion(), p.GetToolkitMinorVersion());
    wxLogVerbose(wxT(" * Architecture: %s"), p.GetArchName().c_str());
    wxLogVerbose(wxT(" * Endianness: %s"), p.GetEndiannessName().c_str());
    wxLogVerbose(wxT(" * WX ID: %s"), p.GetPortIdName().c_str());
    wxLogVerbose(wxT(" * WX Version: %d.%d.%d.%d"), wxMAJOR_VERSION, wxMINOR_VERSION, wxRELEASE_NUMBER, wxSUBRELEASE_NUMBER);
    return;
} //MyApp::LogPlatform



OwwlReaderTimer::OwwlReaderTimer(MyFrame* f, unsigned int pollInt) : wxTimer()
{
    m_pollInterval = pollInt * 1000;
    OwwlReaderTimer::m_frame = f;
    return;
} // OwwlReaderTimer::OwwlReaderTimer
 
void OwwlReaderTimer::Notify()
{
    wxLogVerbose("Readertimer:Notify");
#if 1
    if(NULL != m_frame)
    {
        wxLogVerbose("Latitude=%3.4f", m_frame->m_connection->latitude);
        wxLogVerbose("Longitude=%3.4f", m_frame->m_connection->longitude);

        if(NULL != m_frame->m_connection)
        {
            int retval = owwl_read(m_frame->m_connection);
            switch(retval)
            {
                case Owwl_Read_Error:
                    wxLogVerbose("Protocol Error");
                    // drop thru to disconnect break;
                case Owwl_Read_Disconnect:
                    wxLogVerbose("Server Disconnect");
                    m_frame->SetStatusText("Server disconnect");
                    /* Try to reconnect */
                    /* but for now close up */
                    sock_close(m_frame->m_s);
                    m_frame->m_s = -1;
                    g_connection = m_frame->m_connection = NULL;
                    break;
                case Owwl_Read_Again:
                    wxLogVerbose("Read Again");
                    retval = owwl_tx_poll_servers(m_frame->m_connection);
                    if(-1==retval)wxLogVerbose("owwl_tx_poll_servers failed");
                    break;
                case Owwl_Read_Read_And_Decoded:
                    wxLogVerbose("Read And Decode");
                    retval = owwl_tx_build(m_frame->m_connection, OWW_TRX_MSG_WSDATA, 
                                           &(m_frame->buff));
                    if(-1==retval)wxLogVerbose("owwl_tx_build failed");
                    retval = owwl_tx(m_frame->m_connection, &(m_frame->buff));
                    if(-1==retval)wxLogVerbose("owwl_tx failed");
                    break;
                default:
                    wxLogVerbose("Read Default");
                    break;
            } //switch(owwl_read(m_frame->m_connection))
        } //if(NULL != m_frame->m_connection)
        else
        {
            // try to reconnect... someday...
        }
    } //if(NULL != m_frame)
#endif
    return;
} //OwwlReader::Notify

void OwwlReaderTimer::start()
{
    wxTimer::Start(m_pollInterval);
    return;
} // OwwlReader::start




RenderTimer::RenderTimer(MyCanvas *canvas) : wxTimer()
{
    RenderTimer::m_canvas = canvas;
} // RenderTimer c-tor


void RenderTimer::Notify()
{
    wxLogVerbose("RenderTimer:Notify");
    if(NULL == m_canvas) return;

    m_canvas->Refresh();
    if(NULL == m_canvas->m_auxilliaryFrame) return;
    if(NULL == m_canvas->m_auxilliaryFrame->m_grid) return;
    m_canvas->m_auxilliaryFrame->Refresh();
    m_canvas->m_auxilliaryFrame->m_grid->ForceRefresh();

    return;
} //RenderTimer::Notify

void RenderTimer::start()
{
    wxTimer::Start(500);
} //RenderTimer::start



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
    wxClientDC dc( this );
    PrepareDC( dc );
    m_auxilliaryFrame = NULL;

    // try to find the images in the platform specific location
#ifdef __WXGTK__
    wxString dir = "/usr/local/share/oww/pixmaps/";
#elif __WXOSX_COCOA__
    //wxString dir = "/Library/Application Support/Oww/pixmaps/";
    wxString dir = wxGetCwd() + "/pixmaps/";
#elif __WXMSW__
    wxString dir = /*wxGetOSDirectory() + */ "\\Program Files\\Oww\\pixmaps\\";
#else
#error define your platform
#endif

   wxLogVerbose("Looking for images in %s", dir);

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
        if ( !image.LoadFile( dir + wxString(_T("body.jpg"))) )
            wxLogVerbose(_T("Can't load JPG image"));
        else
            body_jpg = wxBitmap( image );

        if ( !image.LoadFile( dir + wxString(_T("bottom1.jpg"))) )
            wxLogVerbose(_T("Can't load JPG image"));
        else
            bottom1_jpg = wxBitmap( image );

        if ( !image.LoadFile( dir + wxString(_T("bottom2.jpg"))) )
            wxLogVerbose(_T("Can't load JPG image"));
        else
            bottom2_jpg = wxBitmap( image );

        if ( !image.LoadFile( dir + wxString(_T("bottom3.jpg"))) )
            wxLogVerbose(_T("Can't load JPG image"));
        else
            bottom3_jpg = wxBitmap( image );

        if ( !image.LoadFile( dir + wxString(_T("bottom4.jpg"))) )
            wxLogVerbose(_T("Can't load JPG image"));
        else
            bottom4_jpg = wxBitmap( image );

        if ( !image.LoadFile( dir + wxString(_T("bottom5.jpg"))) )
            wxLogVerbose(_T("Can't load JPG image"));
        else
            bottom5_jpg = wxBitmap( image );

        if ( !image.LoadFile( dir + wxString(_T("bottom6.jpg"))) )
            wxLogVerbose(_T("Can't load JPG image"));
        else
            bottom6_jpg = wxBitmap( image );

        if ( !image.LoadFile( dir + wxString(_T("bottom7.jpg"))) )
            wxLogVerbose(_T("Can't load JPG image"));
        else
            bottom7_jpg = wxBitmap( image );

        if ( !image.LoadFile( dir + wxString(_T("bottom8.jpg"))) )
            wxLogVerbose(_T("Can't load JPG image"));
        else
            bottom8_jpg = wxBitmap( image );

        if ( !image.LoadFile( dir + wxString(_T("rh.png"))) )
            wxLogVerbose(_T("Can't load PNG image"));
        else
            rh_png = wxBitmap( image );

        if ( !image.LoadFile( dir + wxString(_T("top1.jpg"))) )
            wxLogVerbose(_T("Can't load JPG image"));
        else
            top1_jpg = wxBitmap( image );

        if ( !image.LoadFile( dir + wxString(_T("top2.jpg"))) )
            wxLogVerbose(_T("Can't load JPG image"));
        else
            top2_jpg = wxBitmap( image );

        if ( !image.LoadFile( dir + wxString(_T("top3.jpg"))) )
            wxLogVerbose(_T("Can't load JPG image"));
        else
            top3_jpg = wxBitmap( image );

        if (top1_jpg.IsOk())
        {
             dc.DrawBitmap( top1_jpg, 0, 0 );
        }

        if (body_jpg.IsOk())
        {
            dc.DrawBitmap( body_jpg, 0, top1_jpg.GetHeight() );
        }

        if (bottom1_jpg.IsOk())
        {
            dc.DrawBitmap( bottom1_jpg, 0, top1_jpg.GetHeight() 
                                                + body_jpg.GetHeight());
        }
     }
    else
    {
        wxLogVerbose(wxT("Can't find image files!"));
    }

    m_counter = 0;
    return;
} //MyCanvas c-tor

MyCanvas::~MyCanvas()
{
    m_renderTimer->Stop();
    delete m_renderTimer;
} //MyCanvas d-tor

void MyCanvas::DrawText(wxString str, wxColor fore, wxColor shadow, wxPoint pt)
{
    wxPaintDC dc( this );
    //DoPrepareDC(dc);
    PrepareDC( dc );
#ifdef __WXGTK__
    int fontSz = 12;
#elif __WXOSX_COCOA__
    int fontSz = 16;
#elif __WXMSW__
    int fontSz = 14;
#else
#error define your platform
#endif

    wxFont f = wxFont(fontSz, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, 
                                                        wxFONTWEIGHT_BOLD);
    wxColor fc = dc.GetTextForeground();
    dc.SetFont(f);
    dc.SetTextForeground( shadow );
    dc.DrawText( str, pt.x+2, pt.y+2);
    dc.SetTextForeground( fore );
    dc.DrawText( str, pt.x, pt.y);

    return;
} //MyCanvas::DrawShadowText

void MyCanvas::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
    wxPaintDC dc( this );
    PrepareDC( dc );
    owwl_data *od = NULL;
    int unit;
#ifdef __WXGTK__
    int fontSz = 13;
#elif __WXOSX_COCOA__
    int fontSz = 16;
#elif __WXMSW__
    int fontSz = 12;
#else
#error define your platform
#endif

    wxFont f = wxFont(fontSz, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, 
                                                        wxFONTWEIGHT_BOLD);
    dc.SetFont(f);

    if(m_frame)
    {
        if(m_frame->m_connection)
        {
            float speed = 0.0;
            float gust = 0.0;
            float bearing = 0.0;
            char linebuf[128];

            dc.SetTextForeground( wxT("WHITE") );

            od = owwl_find(m_frame->m_connection, OwwlDev_Wind, 0, 0);
            if(NULL != od)
            {
                int unit_class = owwl_unit_class(od, 0);
                if ((unit_class >= 0) && (unit_class < OWWL_UNIT_CLASS_LIMIT))
                {
                    unit = unit_choices[unit_class];
                }
                speed =   od->val(od, unit, 0);
                gust =    od->val(od, unit, 1);
                bearing = od->val(od, unit, 2);
                
                static int inc_top = 0;
                if(speed >= 0.0 && speed <= 1.0)
                {
                    inc_top = 0;
                }
                else
                {
                    if(speed > 1.0 && speed <= 5.0)
                    {
                        inc_top = inc_top==0 ? 1 : 0;
                    }
                    else
                    {
                        inc_top = 1;
                    }
                }

                if(inc_top)
                {
                    switch (m_counter % 3)
                    {
                        case 0:
                            if (top1_jpg.IsOk())
                            {
                                dc.DrawBitmap( top1_jpg, 0, 0 );
                            }
                            m_counter++;
                            break;
                        case 1:
                            if (top2_jpg.IsOk())
                            {
                                dc.DrawBitmap( top2_jpg, 0, 0 );
                            }
                            m_counter++;
                            break;
                        case 2:
                            if (top3_jpg.IsOk())
                            {
                                dc.DrawBitmap( top3_jpg, 0, 0 );
                            }
                            m_counter = 0;
                    }
                }
                else
                {
                    //Redraw previous top image
                    switch (m_counter)
                    {
                        case 0:
                            if (top1_jpg.IsOk())
                            {
                                dc.DrawBitmap( top1_jpg, 0, 0 );
                            }
                            break;
                        case 1:
                            if (top2_jpg.IsOk())
                            {
                                dc.DrawBitmap( top2_jpg, 0, 0 );
                            }
                            break;
                        case 2:
                            if (top3_jpg.IsOk())
                            {
                                dc.DrawBitmap( top3_jpg, 0, 0 );
                            }
                    }
                }

                if (body_jpg.IsOk())
                    dc.DrawBitmap( body_jpg, 0, top1_jpg.GetHeight());

                switch( lround(bearing/22.5) )
                {
                    case 1:
                    case 2:
                        dc.DrawBitmap( bottom1_jpg, 0, top1_jpg.GetHeight() 
                                                        + body_jpg.GetHeight());
                        break;
                    case 3:
                    case 4:
                        dc.DrawBitmap( bottom2_jpg, 0, top1_jpg.GetHeight()
                                                        + body_jpg.GetHeight());
                        break;
                    case 5:
                    case 6:
                        dc.DrawBitmap( bottom3_jpg, 0, top1_jpg.GetHeight() 
                                                        + body_jpg.GetHeight());
                        break;
                    case 7:
                    case 8:
                        dc.DrawBitmap( bottom4_jpg, 0, top1_jpg.GetHeight() 
                                                        + body_jpg.GetHeight());
                        break;
                    case 9:
                    case 10:
                        dc.DrawBitmap( bottom5_jpg, 0, top1_jpg.GetHeight() 
                                                        + body_jpg.GetHeight());
                        break;
                    case 11:
                    case 12:
                        dc.DrawBitmap( bottom6_jpg, 0, top1_jpg.GetHeight() 
                                                        + body_jpg.GetHeight());
                        break;
                    case 13:
                    case 14:
                        dc.DrawBitmap( bottom7_jpg, 0, top1_jpg.GetHeight() 
                                                        + body_jpg.GetHeight());
                        break;
                    case 15:
                    case 16:
                        dc.DrawBitmap( bottom8_jpg, 0, top1_jpg.GetHeight() 
                                                        + body_jpg.GetHeight());
                        break;
                    default:
                        dc.DrawBitmap( bottom1_jpg, 0, top1_jpg.GetHeight() 
                                                        + body_jpg.GetHeight());
                }

                // Draw Wind Speed on canvas
                DrawText( wxString::Format("%s %s",
                                        od->str(od, linebuf, 128, unit, -1, 0),
                                        owwl_unit_name(od, unit, 0)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(365, 20));
                // Draw Wind Gust Speed on canvas
                DrawText( wxString::Format("%s %s",
                                        od->str(od, linebuf, 128, unit, -1, 1),
                                        owwl_unit_name(od, unit, 1)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(365, 40));
                // Draw Wind Bearing on canvas
                DrawText( wxString::Format("%s %s", 
                                        od->str(od, linebuf, 128, unit, -1, 2),
                                        owwl_unit_name(od, unit, 2)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(365, 60));
            }
            else
            {
                wxLogVerbose("MyCanvas::OnPaint od OwwDev_Wind == NULL");
            }

            od = owwl_find(m_frame->m_connection, OwwlDev_Humidity, 0, 0);
            if(NULL != od)
            {
                if (rh_png.IsOk())
                {
                    dc.DrawBitmap( rh_png, 300, 180 );
                }
                DrawText( wxString::Format("%s%s", 
                                        od->str(od, linebuf, 128, unit, -1, 0),
                                        owwl_unit_name(od, unit, 0)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(365, 180));
            }
            else
            {
                wxLogVerbose("MyCanvas::OnPaint od OwwDev_Humidity == NULL");
            }

            od = owwl_find(m_frame->m_connection, OwwlDev_Temperature, 
                                                    OwwlTemp_Thermometer, 0);
            if(NULL != od)
            {
                DrawText( wxString::Format("%s%s", 
                                    od->str(od, linebuf, 128, unit, -1, 0),
                                    owwl_unit_name(od, unit, 0)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(25, 125));
            }
            else
            {
                wxLogVerbose("MyCanvas::OnPaint od OwwDev_Temperature == NULL");
            }

            dc.SetTextForeground( wxT("YELLOW") );
            od = owwl_find(m_frame->m_connection, OwwlDev_Temperature, OwwlTemp_Humidity, 0);
            if(NULL != od)
            {
                DrawText( wxString::Format("Trh: %s %s", 
                            od->str(od, linebuf, 128, unit, -1, 0),
                            owwl_unit_name(od, unit, 0)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(280, 260));
            }
            else
            {
                wxLogVerbose("MyCanvas::OnPaint od OwwDev_Temperature/OwwlTemp_Humidity == NULL");
            }

            od = owwl_find(m_frame->m_connection, OwwlDev_Temperature, OwwlTemp_Barometer, 0);
            if(NULL != od)
            {
                DrawText( wxString::Format("Tb: %s %s", 
                            od->str(od, linebuf, 128, unit, -1, 0),
                            owwl_unit_name(od, unit, 0)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(280, 290));
            }
            else
            {
                wxLogVerbose("MyCanvas::OnPaint od OwwDev_Temperature/OwwlTemp_Barometer == NULL");
            }

            od = owwl_find(m_frame->m_connection, OwwlDev_Barometer, 0, 0);
            if(NULL != od)
            {
                DrawText(wxString::Format("BP: %s %s", 
                                        od->str(od, linebuf, 128, unit, 3, 0),
                                        owwl_unit_name(od, unit, 0)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(280,320));
            }
            else
            {
                wxLogVerbose("MyCanvas::OnPaint od OwwDev_Barometer == NULL");
            }

            dc.SetTextForeground( wxT("RED") );
            od = owwl_find(m_frame->m_connection, OwwlDev_Rain, 0, 0);
            if(NULL != od)
            {
                wxDateTime rain_time = wxDateTime(
                                        od->device_data.rain.rain_reset_time);
                DrawText( wxString::Format("Rain: %s %s since %s", 
                                        od->str(od, linebuf, 128, unit, -1, 0),
                                        owwl_unit_name(od, unit, 0),
                                        rain_time.FormatTime()),
                            wxT("YELLOW"), wxT("BLACK"), wxPoint(25, 360));
                DrawText( wxString::Format("(%s %s)", 
                                        od->str(od, linebuf, 128, unit, -1, 2),
                                        owwl_unit_name(od, unit, 2)),
                            wxT("YELLOW"), wxT("BLACK"), wxPoint(325, 360));
            }
            else
            {
                wxLogVerbose("MyCanvas::OnPaint od OwwDev_Rain == NULL");
            }

            dc.SetBrush( wxBrush( wxT("white"), wxSOLID ) );
            dc.SetPen( *wxBLACK_PEN );
            dc.DrawCircle( 325, 120, 25);
            //dc.SetBrush( *wxWHITE_BRUSH );
            //dc.SetPen( *wxRED_PEN );
            dc.DrawRectangle( 322, 50, 5, 10 );

            wxString now = wxNow ();
            m_frame->SetStatusText(now, 1);
        }
        else
        {
            if (top1_jpg.IsOk())
            {
                 dc.DrawBitmap( top1_jpg, 0, 0 );
            }

            if (body_jpg.IsOk())
            {
                dc.DrawBitmap( body_jpg, 0, top1_jpg.GetHeight() );
            }

            if (bottom1_jpg.IsOk())
            {
                dc.DrawBitmap( bottom1_jpg, 0, top1_jpg.GetHeight() 
                                                    + body_jpg.GetHeight());
            }
            m_frame->SetTitle(wxString::Format(wxT("%s"), "Oww")); 

            wxLogVerbose("MyCanvas::OnPaint m_conn==NULL");
        }// if(m_connection)

    }// if(m_frame)


#if 0
    wxScopedPtr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
    wxGraphicsFont gf = gc->CreateFont(wxNORMAL_FONT->GetPixelSize().y, wxT("WHITE") );

    gc->SetFont(gf);
    //dc.SetTextForeground( wxT("WHITE") );
    wxString now = wxNow ();
    gc->DrawText (now, 100, 10);
    gc->DrawText( wxString::Format("%2.1f C", t), 30, 130 );

    gc->DrawText( wxString::Format("%2.1f m/s", s), 365, 20 );
    gc->DrawText( wxString::Format("%2.1f gusts", g), 365, 40 );
    gc->DrawText( wxString::Format("%2.1f bearing", b), 365, 60 );

    dc.SetBrush( wxBrush( wxT("white"), wxSOLID ) );
    dc.SetPen( *wxBLACK_PEN );
    dc.DrawCircle( 350, 100, 30);

    if (gc)
    {
        // make a path that contains a circle and some lines
        gc->SetPen( *wxRED_PEN );
        wxGraphicsPath path = gc->CreatePath();
        path.AddCircle( 50.0, 50.0, 50.0 );
        path.MoveToPoint(0.0, 50.0);
        path.AddLineToPoint(100.0, 50.0);
        path.MoveToPoint(50.0, 0.0);
        path.AddLineToPoint(50.0, 100.0 );
        path.CloseSubpath();
        //path.AddRectangle(25.0, 25.0, 50.0, 50.0);
        path.AddRoundedRectangle(25.0, 25.0, 50.0, 50.0, 10.3);
        gc->StrokePath(path);

        //delete gc;
    }
#endif
} //MyCanvas::OnPaint

enum wxShadowDirection
{
    wxLeftAndUp,
    wxRightAndUp,
    wxDownAndLeft,
    wxDownAndRight
};
enum wxShadowModes
{
    wxShadowPixels,
    wxShadowPercent,
    wxShadowAnotherWay
};

#if 1
class wxShadowDC : wxDC
{
public:
    void SetTextShadowOffset(const int & sz, enum wxShadowModes mode = wxShadowPixels)
    {
        m_shadowOffset = sz;
        m_shadowOffsetMode = mode;
    }

    int GetTextShadowOffset(void)
    {
        return m_shadowOffset;
    }

    void SetTextShadowColour(const wxColour & colour)
    {
        m_shadowColour = colour;
    }

    wxColour GetTextShadowColour(void)
    {
        return m_shadowColour;
    }

    void DrawShadowText(wxString str, wxPoint pt)
    {
        wxPoint shadowPt = pt;
        wxColour foreColour = GetTextForeground();
        wxFont f;
        f.GetPointSize();
        switch(m_shadowOffsetMode)
        {
            case wxShadowPixels:
                shadowPt.x = shadowPt.x + m_shadowOffset;
                shadowPt.y = shadowPt.y + m_shadowOffset;
                break;
            case wxShadowPercent:
                {
                    int offsetPxls = ((float)m_shadowOffset * (float)GetCharHeight() / 100.0);
                    offsetPxls = (0==offsetPxls)?1:offsetPxls;
                    shadowPt.x = shadowPt.x + offsetPxls;
                    shadowPt.y = shadowPt.y + offsetPxls;
                }
                break;
            default:
                break;
        }

        SetTextForeground(m_shadowColour);
        DrawText(str, shadowPt);

        SetTextForeground(foreColour);
        DrawText(str, pt);
    }
    

private:
    wxColour m_shadowColour;
    int      m_shadowOffset;
    enum wxShadowDirection m_shadowOffsetDir;
    enum wxShadowModes m_shadowOffsetMode;
};
#endif



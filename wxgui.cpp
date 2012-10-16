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
#include <errno.h>
#include <sys/time.h>
#else
#include <windows.h>
#include <winsock.h>
//#include <Ws2tcpip.h>
#endif
// app include
#include "wxgui.h"

#ifdef WIN32
#define lround(num) ( (long)(num > 0 ? num + 0.5 : ceil(num - 0.5)) )
#endif


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

wxString g_VersionStr = VERSIONSTR;
owwl_conn *g_connection;


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


wxString default_unit_names[] = {"<Metric>", "<Imperial>", "<Alt1>", "<Alt2>"};
int unit_choices[OWWL_UNIT_CLASS_LIMIT];

//-----------------------------------------------------------------------------
// RenderTimer
//-----------------------------------------------------------------------------
class RenderTimer : public wxTimer
{
public:
    RenderTimer(void);
    RenderTimer(MyFrame * f);
    void Notify();
    void start();
private:
    MyFrame * m_frame;
};


//-----------------------------------------------------------------------------
// MyAuxilliaryFrame
//-----------------------------------------------------------------------------

class MyAuxilliaryFrame : public wxDialog
{
public:
    MyAuxilliaryFrame(wxWindow *parent, const wxString& desc)
    {
        Create(parent, desc); 
    }
    ~MyAuxilliaryFrame();

    wxGrid *m_grid;
    int UpdateCellsUnits(void);
#ifdef __WXGTK__
    RenderTimer *m_renderAuxTimer;
#endif

private:
    wxStatusBar * m_statusBar;
    enum gridColumns {gridColName, gridColData, gridColValue, gridColUnit};
    int InitPopulateCells(void);
    int PopulateCellVals(void);

    bool Create(wxWindow *parent, const wxString& desc)
    {
        if (!wxDialog::Create(parent, wxID_ANY, desc, 
                    wxDefaultPosition, wxDefaultSize,
                    wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER|wxMAXIMIZE_BOX)))
        {
            return false;
        }

        m_grid = (wxGrid*)NULL;
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
        m_grid->SetRowLabelSize(wxGRID_AUTOSIZE);
        m_grid->SetColLabelSize(wxGRID_AUTOSIZE);
        m_grid->AutoSize();
        SetClientSize(m_grid->GetSize());

#ifdef __WXGTK__
        m_renderAuxTimer = new RenderTimer();
        m_renderAuxTimer->start();
#endif

        Show();
        return true;
    }

    void OnPaint(wxPaintEvent& WXUNUSED(event))
    {
	wxPaintDC dc(this);
        PopulateCellVals();
#ifndef __WXOSX_COCOA__
        m_grid->AutoSize();
#endif
        SetClientSize(m_grid->GetSize());
    }

    DECLARE_EVENT_TABLE()
};

MyAuxilliaryFrame::~MyAuxilliaryFrame()
{
#ifdef __WXGTK__
    m_renderAuxTimer->Stop();
    delete m_renderAuxTimer;
#endif
}



int MyAuxilliaryFrame::InitPopulateCells()
{
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
}


int MyAuxilliaryFrame::UpdateCellsUnits()
{
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
}

int MyAuxilliaryFrame::PopulateCellVals(void)
{
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
    return 0;
}


// ----------------------------------------------------------------------------
// MySettingsDialogy
// ----------------------------------------------------------------------------
class MySettingsDialog: public wxPropertySheetDialog
{
DECLARE_CLASS(MySettingsDialog)
public:
    MySettingsDialog(wxWindow* parent, int dialogType);
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
    wxChoice          *browserChoice;
    wxTextCtrl        *urlCmdText;


protected:
    enum {
        ID_SERVER_TEXT= 100,
        ID_PORT_SPIN,
        ID_POLL_SPIN,
        ID_LAUNCH_CHECK,
        ID_UNITS_CHOICE,
        ID_ANIMATE_CHECK,
        ID_BROWSER_CHOICE,
        ID_URLCMD_TEXT
    };

    wxImageList*    m_imageList;

DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// MySetupDialogy
// ----------------------------------------------------------------------------

class MySetupDialog: public wxDialog
{
public:
 
    MySetupDialog ( wxWindow * , wxWindowID , wxString const & , 
                        wxPoint const & , wxSize const & , long );

    wxTextCtrl * dialogText;
    wxString GetText();
 
private:
    wxButton *m_btnOk,
             *m_btnCancel,
             *m_btnDelete;
 
    void OnButton( wxCommandEvent & event );
 
    DECLARE_EVENT_TABLE()
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


enum
{
    Menu_SubMenu = 450,
    Menu_SubMenu_Radio0,
    Menu_SubMenu_Radio1,
    Menu_SubMenu_Radio2,
    Menu_SubMenu_Radio3,
    DIALOGS_PROPERTY_SHEET,
    DIALOGS_PROPERTY_SHEET_TOOLBOOK,
    DIALOGS_PROPERTY_SHEET_BUTTONTOOLBOOK,

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
    ~MyFrame()
    {
#ifdef __WXMSW__
        WSACleanup();
#endif
    };

    void OnAbout( wxCommandEvent &event );
    void OnAuxilliary( wxCommandEvent &event );
    void OnMap( wxCommandEvent &event );
    void OnMenuToggleUnits( wxCommandEvent &event );
#if 0
    void OnSetup( wxCommandEvent &event );
    void OnDevices( wxCommandEvent &event );
#endif
    void OnQuit( wxCommandEvent &event );

    wxMenu            *menuImage;
    wxMenu            *subMenu;
    MyCanvas          *m_canvas;
    MyAuxilliaryFrame *m_auxilliaryFrame;
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

private:
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
};



// ============================================================================
// implementations
// ============================================================================

//-----------------------------------------------------------------------------
// MyImageFrame
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

#if 0
char g_tempStr[50];
char g_windStr[50];
#endif

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
        subMenu->Check(Menu_SubMenu_Radio0, (m_units==OwwlUnit_Metric));
        subMenu->Check(Menu_SubMenu_Radio1, (m_units==OwwlUnit_Imperial));
        subMenu->Check(Menu_SubMenu_Radio2, (m_units==OwwlUnit_Alt1));
        subMenu->Check(Menu_SubMenu_Radio3, (m_units==OwwlUnit_Alt2));
    }
    return;
}


IMPLEMENT_DYNAMIC_CLASS( MyFrame, wxFrame )
BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(ID_ABOUT, MyFrame::OnAbout)
    EVT_MENU(ID_QUIT,  MyFrame::OnQuit)
    EVT_MENU(ID_AUXILLIARY, MyFrame::OnAuxilliary)
    EVT_MENU(ID_MAP,   MyFrame::OnMap)
    EVT_MENU(ID_MESSAGES,  MyFrame::OnMenuToggleUnits)
#if 0
    EVT_MENU(ID_SETUP, MyFrame::OnSetup)
    EVT_MENU(ID_DEVICES, MyFrame::OnDevices)
#endif

    EVT_MENU(Menu_SubMenu_Radio0, MyFrame::OnMenuSetUnits)
    EVT_MENU(Menu_SubMenu_Radio1, MyFrame::OnMenuSetUnits)
    EVT_MENU(Menu_SubMenu_Radio2, MyFrame::OnMenuSetUnits)
    EVT_MENU(Menu_SubMenu_Radio3, MyFrame::OnMenuSetUnits)

    EVT_MENU(DIALOGS_PROPERTY_SHEET,                MyFrame::OnPropertySheet)
    EVT_MENU(DIALOGS_PROPERTY_SHEET_TOOLBOOK,       MyFrame::OnPropertySheet)
    EVT_MENU(DIALOGS_PROPERTY_SHEET_BUTTONTOOLBOOK, MyFrame::OnPropertySheet)

END_EVENT_TABLE()


MyFrame::MyFrame()
    : wxFrame( (wxFrame *)NULL, wxID_ANY, wxT("Oww"), 
                wxPoint(20, 20), 
#ifdef __WXGTK__
                wxSize(474, 441+40),
#else
                wxSize(474, 441),
#endif
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
    m_pollInterval = 5;
    m_s = -1;
    m_auxilliaryFrame = NULL;
    m_units = OwwlUnit_Imperial;
    m_browser = 0;
    m_mapurl = wxEmptyString;

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
        (void)wxMessageBox(_("The Winsock dll not found!"),
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
        (void)wxMessageBox(wxString::Format("The dll do not support the Winsock version %u.%u!", 
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
    menuImage->Append(ID_MESSAGES, wxT("Toggle Units"), "Swap Meteric and Imperial");
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
    menuImage->AppendSeparator();
    menuImage->Append(DIALOGS_PROPERTY_SHEET, wxT("Setup"), "Edit Preferences");
#if 0
    menuImage->Append(DIALOGS_PROPERTY_SHEET_TOOLBOOK, wxT("&Toolbook sheet\tShift-Ctrl-T"));
    menuImage->Append(ID_SETUP, wxT("Setup"), "Edit Preferences");
    menuImage->Append(ID_DEVICES, "Devices", "Configure devices");
#endif
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
    Refresh();

    m_canvas = new MyCanvas( this, wxID_ANY, wxPoint(0,0), wxSize(474,441) );
    Show();

    m_renderTimer = new RenderTimer(this);
    m_renderTimer->start();

    m_readerTimer = new OwwlReaderTimer(this);
    m_readerTimer->start();

    if(0 == InitServerConnection())
    {
        SetTitle(wxString::Format(wxT("%s://%s:%d"), GetTitle(), m_hostname, (int)m_port));
    }
    return;
}

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
                (void)wxMessageBox(wxString::Format("gethostbyname=%d", WSAGetLastError()),
			       	"One wire Weather", wxICON_INFORMATION | wxOK );
	    }
	}
	else
	{    
	    addr_in.sin_addr.s_addr = inet_addr(m_hostname.c_str());
            host = gethostbyaddr((const char *)&addr_in, sizeof(struct sockaddr_in), AF_INET);
            if(NULL == host) 
	    {
                (void)wxMessageBox(wxString::Format("gethostbyaddr=%d", WSAGetLastError()),
			       	"One wire Weather", wxICON_INFORMATION | wxOK );
	    }
        }
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
                g_connection = m_connection = owwl_new_conn(m_s, NULL);
                if (connect(m_s, address, addr_len) == 0)
                {
                    int retval = owwl_read(m_connection);
                    switch(retval)
                    {
                        case Owwl_Read_Error:
                            wxLogStatus("Protocol error");
                            retval = -1;
                            break;
                        case Owwl_Read_Disconnect:
                            wxLogStatus("Server disconnect");
                            _close(m_s);
                            m_s = -1;
                            g_connection = m_connection = NULL;
                            retval = -1;
                            break;
                        case Owwl_Read_Again:
                            wxLogStatus("Read again");
                            retval = -1;
                            break;
                        case Owwl_Read_Read_And_Decoded:
                            //SetStatusText("Read & Decode");
                            break;
                        default:
                            retval = -1;
                            wxLogStatus("Read default");
                            break;
                    }
                    if(m_connection)
                    {
                        owwl_foreach(m_connection, print_data, NULL);
                    }
                }
                else
                {
                    wxLogStatus("Error: connect failed %d", errno);
                    owwl_free(m_connection);
                    _close(m_s);
                    m_s = -1;
                    g_connection = m_connection = NULL;
                    retval = -1;
                }
            }
            else
            {
                wxLogStatus("Error: s<0 %d", errno);
                retval = -1;
            }
        }
        else
        {
            wxLogStatus(wxT("Unable to resolve host name %s"), m_hostname);
            retval = -1;
        }
    }
    return retval;
}



void MyFrame::OnMenuSetUnits(wxCommandEvent& event)
{
    int unit = event.GetId() - Menu_SubMenu_Radio0;

    if(NULL != m_connection)
    {
        changeUnits(unit);
    }
    if(NULL != m_auxilliaryFrame)
    {
        m_auxilliaryFrame->UpdateCellsUnits();
    }
    return;
}


void MyFrame::OnQuit( wxCommandEvent &WXUNUSED(event) )
{
    owwl_free(m_connection);
    _close(m_s);
    m_s = -1;
    g_connection = m_connection = NULL;
    m_renderTimer->Stop();
    delete m_renderTimer;
    Close( true );
}


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
    info.SetName(_("Oww"));
    info.SetDescription(_("One wire Weather"));
    info.SetWebSite(_("oww.sourceforge.net"), _("Oww website"));
    info.SetVersion(g_VersionStr);
    info.SetCopyright(_T("(C) 2012 Mark Clayton <mark_clayton@users.sourceforge.net>"));

    wxAboutBox(info);
    
#endif
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


#if 0
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
    wxButton *b = new wxButton(this, wxID_OK, _("OK"), p, wxDefaultSize);
    p.x += 110;
    wxButton *c = new wxButton(this, wxID_CANCEL, _("Cancel"), p, wxDefaultSize);

    SetEscapeId(wxID_CANCEL);

}

void MyFrame::OnSetup( wxCommandEvent &WXUNUSED(event) )
{
    wxString dialogText;
    MySetupDialog setupDialog(this, -1, _("Your very own dialog"),
                              wxPoint(100, 100), wxSize(200, 200));
    setupDialog.ShowModal();
}
#endif

void MyFrame::OnAuxilliary(wxCommandEvent &WXUNUSED(event))
{
    m_auxilliaryFrame = new MyAuxilliaryFrame(this, "Auxilliary Data");
}

void MyFrame::OnMap( wxCommandEvent &WXUNUSED(event) )
{
    if(NULL != m_connection)
    {
        char command[2048];
        sprintf(command, "open /Applications/Safari.app/Contents/MacOS/Safari " + m_mapurl, m_connection->latitude, m_connection->longitude);

        wxArrayString output;
        wxExecute(wxString(command), output);
    }
    else
    {
        (void)wxMessageBox("Can not map a server when you're not connected to a server",
                        "One wire Weather",
                        wxICON_INFORMATION | wxOK );
    }
}


void MyFrame::OnMenuToggleUnits( wxCommandEvent &WXUNUSED(event) )
{
    int i;
    if(NULL != m_connection)
    {
        changeUnits((unit_choices[i]==OwwlUnit_Imperial) ? OwwlUnit_Metric : OwwlUnit_Imperial);
        //for (i=0; i<OWWL_UNIT_CLASS_LIMIT; ++i)
        //{
        //    unit_choices[i] = (unit_choices[i]==OwwlUnit_Imperial)
        //                                   ? OwwlUnit_Metric : OwwlUnit_Imperial;
        //}
    }
    if(NULL != m_auxilliaryFrame)
    {
        m_auxilliaryFrame->UpdateCellsUnits();
    }
}

#if 0
void MyFrame::OnDevices(wxCommandEvent& WXUNUSED(event))
{
    new MyDevicesFrame(this, "Devices");
}
#endif

void MyFrame::OnPropertySheet(wxCommandEvent& event)
{
    MySettingsDialog dialog(this, event.GetId());

    dialog.serverText->SetValue(m_hostname);
    dialog.portSpin->SetValue(m_port);
    dialog.pollSpin->SetValue(m_pollInterval);
    dialog.launchAtStart->SetValue(true);
    dialog.unitsChoice->SetSelection(unit_choices[0]);
    dialog.animateDisplay->SetValue(true);
    dialog.browserChoice->SetSelection(m_browser);
    dialog.urlCmdText->SetValue(m_mapurl);

    switch(dialog.ShowModal())
    {
        case wxID_CANCEL:
            //(void)wxMessageBox("Cancel", "One wire Weather", wxICON_INFORMATION | wxOK );
            break;
        case wxID_OK:
            m_hostname = dialog.serverText->GetValue();
            m_port = dialog.portSpin->GetValue();
            m_pollInterval = dialog.pollSpin->GetValue();
            m_launchAtStart = dialog.launchAtStart->GetValue();
            m_units = dialog.unitsChoice->GetSelection();
            m_animateDisplay = dialog.animateDisplay->GetValue();
            m_browser = dialog.browserChoice->GetSelection();
            m_mapurl = dialog.urlCmdText->GetValue();
            m_config = wxConfigBase::Get();
            m_config->Write("server", m_hostname);
            m_config->Write("port", m_port);
	    m_config->Write("pollInterval", m_pollInterval);
            m_config->Write("launchStStart", m_launchAtStart);
            m_config->Write("units", m_units);
            changeUnits(m_units);
            m_config->Write("anumateDisplay", m_animateDisplay);
            m_config->Write("browser", m_browser);
            m_config->Write("mapurl", m_mapurl);
            //(void)wxMessageBox("OK", "One wire Weather", wxICON_INFORMATION | wxOK );
            break;
        default:
            (void)wxMessageBox("default", "One wire Weather", wxICON_INFORMATION | wxOK );
    }
}


// ----------------------------------------------------------------------------
// SettingsDialog
// ----------------------------------------------------------------------------

IMPLEMENT_CLASS(MySettingsDialog, wxPropertySheetDialog)

BEGIN_EVENT_TABLE(MySettingsDialog, wxPropertySheetDialog)

END_EVENT_TABLE()

MySettingsDialog::MySettingsDialog(wxWindow* win, int dialogType)
{
    SetExtraStyle(wxDIALOG_EX_CONTEXTHELP|wxWS_EX_VALIDATE_RECURSIVELY);

    int tabImage1 = -1;
    int tabImage2 = -1;
    int tabImage3 = -1;

    bool useToolBook = (dialogType == DIALOGS_PROPERTY_SHEET_TOOLBOOK || dialogType == DIALOGS_PROPERTY_SHEET_BUTTONTOOLBOOK);
    int resizeBorder = wxRESIZE_BORDER;

    if (useToolBook)
    {
        resizeBorder = 0;
        tabImage1 = 0;
        tabImage2 = 1;
        tabImage3 = 0;

        int sheetStyle = wxPROPSHEET_SHRINKTOFIT;
        if (dialogType == DIALOGS_PROPERTY_SHEET_BUTTONTOOLBOOK)
            sheetStyle |= wxPROPSHEET_BUTTONTOOLBOOK;
        else
            sheetStyle |= wxPROPSHEET_TOOLBOOK;

        SetSheetStyle(sheetStyle);
        SetSheetInnerBorder(0);
        SetSheetOuterBorder(0);

        // create a dummy image list with a few icons
        const wxSize imageSize(32, 32);

        m_imageList = new wxImageList(imageSize.GetWidth(), imageSize.GetHeight());
        m_imageList->
            Add(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_OTHER, imageSize));
        m_imageList->
            Add(wxArtProvider::GetIcon(wxART_QUESTION, wxART_OTHER, imageSize));
        m_imageList->
            Add(wxArtProvider::GetIcon(wxART_WARNING, wxART_OTHER, imageSize));
        m_imageList->
            Add(wxArtProvider::GetIcon(wxART_ERROR, wxART_OTHER, imageSize));
    }
    else
        m_imageList = NULL;

    Create(win, wxID_ANY, _("Preferences"), wxDefaultPosition, wxSize(500,300),
        wxDEFAULT_DIALOG_STYLE| (int)wxPlatform::IfNot(wxOS_WINDOWS_CE, resizeBorder)
    );

    // If using a toolbook, also follow Mac style and don't create buttons
    if (!useToolBook)
        CreateButtons(wxOK | wxCANCEL |
                        (int)wxPlatform::IfNot(wxOS_WINDOWS_CE, wxHELP)
    );

    wxBookCtrlBase* notebook = GetBookCtrl();
    notebook->SetImageList(m_imageList);

    wxPanel* serverSettings ;
    wxPanel* displaySettings;
    wxPanel* launchSettings ;

    serverSettings = CreateServerSettingsPage(notebook);
    displaySettings = CreateDisplaySettingsPage(notebook);
    launchSettings = CreateLaunchSettingsPage(notebook);

    notebook->AddPage(serverSettings, _("Server"), true, tabImage1);
    notebook->AddPage(displaySettings, _("Display"), false, tabImage2);
    notebook->AddPage(launchSettings, _("URL Launch"), false, tabImage3);

    LayoutDialog();
}

MySettingsDialog::~MySettingsDialog()
{
    delete m_imageList;
}

wxPanel* MySettingsDialog::CreateServerSettingsPage(wxWindow* parent)
{
    wxPanel* panel = new wxPanel(parent, wxID_ANY);

    wxBoxSizer *topSizer = new wxBoxSizer( wxVERTICAL );

    // Connect to server on startup
    wxBoxSizer* itemSizer = new wxBoxSizer( wxVERTICAL );
    serverText = new wxTextCtrl(panel, ID_SERVER_TEXT, wxEmptyString, wxDefaultPosition, wxSize(300, -1), wxTE_LEFT);
    serverText->SetFocus();
    portSpin = new wxSpinCtrl(panel, ID_PORT_SPIN, wxEmptyString, wxDefaultPosition, wxSize(100, -1), wxSP_ARROW_KEYS, 7000, 65500, 8899);
    pollSpin = new wxSpinCtrl(panel, ID_POLL_SPIN, wxEmptyString, wxDefaultPosition, wxSize(50, -1), wxSP_ARROW_KEYS, 1, 65, 5);
    launchAtStart = new wxCheckBox(panel, ID_LAUNCH_CHECK, _("Connect on startup"), wxDefaultPosition, wxDefaultSize);
    itemSizer->Add(new wxStaticText(panel, wxID_STATIC, _("Server:")), 0, wxALIGN_CENTER_HORIZONTAL, 5);
    itemSizer->Add(serverText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 2);
    itemSizer->Add(new wxStaticText(panel, wxID_STATIC, _("Port : ")), 0, wxALIGN_CENTER_HORIZONTAL, 5);
    itemSizer->Add(portSpin, 0, wxALL|wxALIGN_CENTER_VERTICAL, 2);
    itemSizer->Add(new wxStaticText(panel, wxID_STATIC, _("Poll Interval (seconds) : ")), 0, wxALIGN_CENTER_HORIZONTAL, 5);
    itemSizer->Add(pollSpin, 0, wxALL|wxALIGN_CENTER_VERTICAL, 2);
    itemSizer->Add(launchAtStart, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    topSizer->Add(itemSizer, 1, wxGROW|wxALIGN_CENTRE|wxALL, 5 );
    panel->SetSizerAndFit(topSizer);

    return panel;
}

wxPanel* MySettingsDialog::CreateDisplaySettingsPage(wxWindow* parent)
{
    wxPanel* panel = new wxPanel(parent, wxID_ANY);

    wxBoxSizer *topSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer *itemSizer = new wxBoxSizer( wxVERTICAL );

    itemSizer->Add(new wxStaticText(panel, wxID_ANY, _("Units:")), 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    // Display Units: Metric, Imperial, Alt 1, Alt 2
    wxArrayString unitChoices;
    unitChoices.Add(wxT("Metric"));
    unitChoices.Add(wxT("Imperial"));
    unitChoices.Add(wxT("Alt 1"));
    unitChoices.Add(wxT("Alt 2"));
    unitsChoice = new wxChoice(panel, ID_UNITS_CHOICE, wxDefaultPosition, wxDefaultSize, unitChoices);
    unitsChoice->SetSelection(1);
    unitsChoice->SetFocus();
    animateDisplay = new wxCheckBox(panel, ID_ANIMATE_CHECK, _("Animate"), wxDefaultPosition, wxDefaultSize);
    itemSizer->Add(unitsChoice, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    itemSizer->Add(animateDisplay, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    topSizer->Add(itemSizer, 0, wxGROW|wxALL, 5);
    panel->SetSizerAndFit(topSizer);

    return panel;
}



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
    browserChoice = new wxChoice(panel, ID_BROWSER_CHOICE, wxDefaultPosition, wxDefaultSize, browserChoices);
    browserChoice->SetSelection(1);
    browserChoice->SetFocus();
    urlCmdText = new wxTextCtrl(panel, ID_URLCMD_TEXT, wxEmptyString, wxDefaultPosition, wxSize(400, -1), wxTE_LEFT);

    itemSizer2->Add(new wxStaticText(panel, wxID_ANY, _("Browser:")), 0, wxALL|wxALIGN_CENTER_VERTICAL, 2);
    itemSizer2->Add(browserChoice, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5);
    itemSizer2->Add(new wxStaticText(panel, wxID_ANY, _("Map URL:")), 0, wxALL|wxALIGN_CENTER_VERTICAL, 2);
    itemSizer2->Add(urlCmdText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 2);
    styleSizer->Add(itemSizer2, 0, wxGROW|wxALL, 2);

    topSizer->Add( item0, 1, wxGROW|wxALIGN_CENTRE|wxALL, 5 );
    //topSizer->AddSpacer(5);

    panel->SetSizerAndFit(topSizer);

    return panel;
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


OwwlReaderTimer::OwwlReaderTimer(MyFrame* f, unsigned int pollInt) : wxTimer()
{
    m_pollInterval = pollInt * 1000;
    OwwlReaderTimer::m_frame = f;
}
 
void OwwlReaderTimer::Notify()
{
#if 1
    if(NULL != m_frame)
    {
        if(NULL != m_frame->m_connection)
        {
            int retval = owwl_read(m_frame->m_connection);
            switch(retval)
            {
                case Owwl_Read_Error:
                    m_frame->SetStatusText("Protocol error");
                    break;
                case Owwl_Read_Disconnect:
                    m_frame->SetStatusText("Server disconnect");
                    _close(m_frame->m_s);
                    m_frame->m_s = -1;
                    g_connection = m_frame->m_connection = NULL;
                    break;
                case Owwl_Read_Again:
                    m_frame->SetStatusText("Read again");
                    break;
                case Owwl_Read_Read_And_Decoded:
                    m_frame->SetStatusText("                  ");
                    break;
                default:
                    m_frame->SetStatusText("Read default");
                    break;
            }
        }
    }
#endif

}

void OwwlReaderTimer::start()
{
    wxTimer::Start(m_pollInterval);
}




RenderTimer::RenderTimer() : wxTimer()
{
    RenderTimer::m_frame = NULL;
}


RenderTimer::RenderTimer(MyFrame * f) : wxTimer()
{
    RenderTimer::m_frame = f;
}

void RenderTimer::Notify()
{
    if(NULL == m_frame)
    {
        return;
    }

    if(NULL != m_frame->m_auxilliaryFrame)
    {
        m_frame->m_auxilliaryFrame->m_grid->ForceRefresh();
        m_frame->m_auxilliaryFrame->Update();
    }
    m_frame->m_canvas->Refresh();
    m_frame->m_canvas->Update();

    return;
}

void RenderTimer::start()
{
    wxTimer::Start(500);
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
    wxClientDC dc( this );
    PrepareDC( dc );
 
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
        wxArrayString array;
        array.Add("Can't find Image files in:");
        array.Add(dir);
        (void)wxMessageBox( wxJoin(array, '\n'),
                        "One wire Weather",
                        wxICON_INFORMATION | wxOK );
        wxLogWarning(wxT("Can't find image files!"));
    }

    m_counter = 0;
    return;
}

MyCanvas::~MyCanvas()
{
}

void MyCanvas::DrawText(wxString str, wxColor fore, wxColor shadow, wxPoint pt)
{
    wxPaintDC dc( this );
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
}

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
#if 0 //WXOWW_SIMULATION
                speed = 3.5;
                gust = 99.9;
                bearing = 180.0;
#else
#if 1
                speed =   od->device_data.wind.speed;
                gust =    od->device_data.wind.gust;
                bearing = od->device_data.wind.bearing;
#else
                speed =   od->val(od, unit, 0);
                gust =    od->val(od, unit, 1);
                bearing = od->val(od, unit, 2);
#endif
#endif
                
                
                static int inc_top = 0;
#if 1
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
#else
                inc_top = 1;
#endif

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

            od = owwl_find(m_frame->m_connection, OwwlDev_Temperature, 
                                                    OwwlTemp_Thermometer, 0);
            if(NULL != od)
            {
                DrawText( wxString::Format("%s%s", 
                                    od->str(od, linebuf, 128, unit, -1, 0),
                                    owwl_unit_name(od, unit, 0)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(25, 125));
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

            od = owwl_find(m_frame->m_connection, OwwlDev_Temperature, OwwlTemp_Barometer, 0);
            if(NULL != od)
            {
                DrawText( wxString::Format("Tb: %s %s", 
                            od->str(od, linebuf, 128, unit, -1, 0),
                            owwl_unit_name(od, unit, 0)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(280, 290));
            }

            od = owwl_find(m_frame->m_connection, OwwlDev_Barometer, 0, 0);
            if(NULL != od)
            {
#if 0
                dc.SetTextForeground( wxT("BLACK") );
                dc.DrawText( wxString::Format("BP: %s %s", 
                                        od->str(od, linebuf, 128, unit, -1, 0),
                                        owwl_unit_name(od, unit, 0)), 282, 322);
                dc.SetTextForeground( wxT("YELLOW") );
                dc.DrawText( wxString::Format("BP: %s %s", 
                                        od->str(od, linebuf, 128, unit, -1, 0),
                                        owwl_unit_name(od, unit, 0)), 280, 320);
#else
                DrawText(wxString::Format("BP: %s %s", 
                                        od->str(od, linebuf, 128, unit, 3, 0),
                                        owwl_unit_name(od, unit, 0)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(280,320));

#endif
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



}




class wxShadowDC:wxDC
{
public:
    void SetTextShadow(const wxColour & colour)
    {
        m_shadowColour = colour;
    }

    wxColour GetTextShadow(void)
    {
        return m_shadowColour;
    }

    void DrawShadowText(wxString str, wxPoint pt)
    {
        wxPoint shadowPt = pt;
        shadowPt.x = shadowPt.x + 2;
        shadowPt.y = shadowPt.y + 2;
        wxColour foreColour = GetTextForeground();
        SetTextForeground(m_shadowColour);
        DrawText(str, shadowPt);

        SetTextForeground(foreColour);
        DrawText(str, pt);
    }
    
private:
    wxColour m_shadowColour;
};

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
#include <wx/frame.h>
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
#include <wx/cmdline.h>
#include <wx/stdpaths.h>
#include <wx/datetime.h>

// system includes
#ifndef __WXMSW__
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
// ifdef HAVE_SYS_UN_H
#include <sys/un.h>
//ifdef HAVE_UNISTD_H
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#define sock_error h_errno
#define sock_close close
#define IOCTL(s,c,a) ioctl(s,c,a)
#else
#include <windows.h>
#include <winsock.h>
//#include <Ws2tcpip.h>
#define lround(num) ( (long)(num > 0 ? num + 0.5 : ceil(num - 0.5)) )
#define sock_error WSAGetLastError()
#define sock_close _close
#define IOCTL(s,c,a) ioctlsocket(s,c,a)
#endif
#include <complex>

// app include
#include "wxgui.h"
#include "heatindex.h"
#include "winddir.h"

class MyFrame;

#ifdef __cplusplus
extern "C" {
#endif
#define HAVE_GETTIMEOFDAY 1

#include "owwl.h"


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

wxString g_VersionStr = VERSIONSTR;
owwl_conn *g_connection;

wxLogWindow * m_logWindow;

#ifndef wxHAS_IMAGES_IN_RESOURCES
    #include "pixmaps/oww_xpm.xpm"
#endif


float convertUnits(float intemp, unsigned int in, unsigned int out)
{
    float outtemp;
    if(in == out)
        outtemp = intemp;
    else
    {
        switch(in)
        {
            case OwwlUnit_Celcius:
                switch(out)
                {
                    case OwwlUnit_Celcius:
                        outtemp = intemp;
                        break;
                    case OwwlUnit_Fahrenheit:
                        outtemp = (intemp * 1.8) + 32;
                        break;
                    case OwwlUnit_Kelvin:
                        outtemp = intemp + 273.15;
                        break;
                    case OwwlUnit_Rankine:
                        outtemp = (intemp + 273.15) * 1.8;
                        break;
                    default:
                        break;
                };
                break;
            case OwwlUnit_Fahrenheit:
                switch(out)
                {
                    case OwwlUnit_Celcius:
                        outtemp = (intemp - 32.0) * 0.5556;
                        break;
                    case OwwlUnit_Fahrenheit:
                        outtemp = intemp;
                        break;
                    case OwwlUnit_Kelvin:
                        outtemp = ((intemp + 459.67) * 0.5556);
                        break;
                    case OwwlUnit_Rankine:
                        outtemp = (intemp + 459.67);
                        break;
                    default:
                        break;
                };
                break;
            case OwwlUnit_Kelvin:
                switch(out)
                {
                    case OwwlUnit_Celcius:
                        outtemp = intemp - 273.15;
                        break;
                    case OwwlUnit_Fahrenheit:
                        outtemp = (intemp * 1.8) - 459.67;
                        break;
                    case OwwlUnit_Kelvin:
                        outtemp = intemp;
                        break;
                    case OwwlUnit_Rankine:
                        outtemp = intemp * (1.8);
                        break;
                    default:
                        break;
                };
                break;
            case OwwlUnit_Rankine:
                switch(out)
                {
                    case OwwlUnit_Celcius:
                        outtemp = (intemp - 491.67) * 0.55556;
                        break;
                    case OwwlUnit_Fahrenheit:
                        outtemp = intemp - 459.67;
                        break;
                    case OwwlUnit_Kelvin:
                        outtemp = intemp * 0.55556;
                        break;
                    case OwwlUnit_Rankine:
                        outtemp = intemp;
                        break;
                    default:
                        break;
                };
                break;
            default:
                break;
        }
    }
    return outtemp;
}

// ============================================================================
// declarations
// ============================================================================
class MyBaroPressure
{
public:
    MyBaroPressure(double reading, time_t time_stamp)
    {
        m_pressureReading = reading;
        m_timeStamp = time_stamp;
    };
    double GetReading() { return m_pressureReading; };
    time_t GetTimeStamp() { return m_timeStamp; };

private:
    double m_pressureReading;
    time_t m_timeStamp;
};

WX_DECLARE_OBJARRAY(MyBaroPressure, ArrayOfPressReadings);

class PressureTendency
{
public:
    PressureTendency() { press_trend = ARRAY_ERROR; };

    enum pressTendValues
    {
        RAPIDLY_RISING  =  4,
        QUICKLY_RISING  =  3,
        RISING          =  2,
        SLOWLY_RISING   =  1,
        STEADY          =  0,
        SLOWLY_FALLING  = -1,
        FALLING         = -2,
        QUICKLY_FALLING = -3,
        RAPIDLY_FALLING = -4,
        ARRAY_ERROR     = -99
    };

    int GetPressureTendency(void);
    void inHg2PressTend(owwl_conn* connection);

private:
    ArrayOfPressReadings m_baroReadings;

    int barometric_change(double past_reading, double current_reading);
    int press_trend;

};

/*                                 hPa          inHg          mmHg
 * Steady, less than               0.1          0.003         0.08
 * Slowly rising or falling     0.15 to 1.5  0.003 to 0.04  0.08 to 1.1
 * Rising or falling             1.6 to 3.5   0.05 to 0.1   1.2 to 2.6
 * Quickly rising or falling     3.6 to 6.0   0.1 to 0.18   2.7 to 4.5
 * Rapidly rising or falling more than 6.0      0.18           4.5
 * (hPa == millibar)
 * To convert inHg to millibars, multiply the inches value by 33.8637526
 * To convert millibars to inHg, multiply the millibar value by 0.0295301
 */
int PressureTendency::barometric_change(double past_reading, double current_reading)
{
    int units = OwwlUnit_InchHg;
    int direction, pressure_tendency;
    double diff = current_reading - past_reading;
    if(0 > diff)
    {
        direction = -1; /*falling*/
    }
    else
    {
        direction = 1; /* rising */
    }
    diff = std::abs(diff);
    switch(units)
    {
        case OwwlUnit_Mbar:
            if(0.1 >= diff)
            {
                pressure_tendency = 0; /* steady */
            }
            else if(1.5  >= diff)
            {
                pressure_tendency = 1; /* slightly */
            }else if(3.5 >= diff)
            {
                pressure_tendency = 2; /* moderately */
            }
            else if(6.0 >= diff)
            {
                pressure_tendency = 3; /* quickly */
            }
            else
            {
                pressure_tendency = 4; /* rapidly */
            }
            break;
        case OwwlUnit_InchHg:
            if(0.003 >= diff)
            {
                pressure_tendency = 0; /* steady */
            }
            else if(0.04 >= diff)
            {
                pressure_tendency = 1; /* slightly */
            }else if(0.1 >= diff)
            {
                pressure_tendency = 2; /* moderately */
            }
            else if(0.18 >= diff)
            {
                pressure_tendency = 3; /* quickly */
            }
            else
            {
                pressure_tendency = 4; /* rapidly */
            }
            break;
        case OwwlUnit_Kpa:
            if(0.08 >= diff)
            {
                pressure_tendency = 0; /* steady */
            }
            else if(1.1 >= diff)
            {
                pressure_tendency = 1; /* slightly */
            }else if(2.6 >= diff)
            {
                pressure_tendency = 2; /* moderately */
            }
            else if(4.5 >= diff)
            {
                pressure_tendency = 3; /* quickly */
            }
            else
            {
                pressure_tendency = 4; /* rapidly */
            }
            break;
        default:
            break;
    }

    return pressure_tendency * direction;
};


// Now that we have MyDirectory declaration in scope we may finish the
// definition of ArrayOfDirectories -- note that this expands into some C++
// code and so should only be compiled once (i.e., don't put this in the
// header, but into a source file or you will get linking errors)
#include <wx/arrimpl.cpp> // This is a magic incantation which must be done!
WX_DEFINE_OBJARRAY(ArrayOfPressReadings);

/*
*/

void PressureTendency::inHg2PressTend(owwl_conn* connection)
{
    owwl_data* od = owwl_find(connection, OwwlDev_Barometer, 0, 0);
    if(NULL != od)
    {
        MyBaroPressure baroReading(od->val(od, OwwlUnit_Imperial, 0),
                                                        connection->data_time);
        m_baroReadings.Add(baroReading);
        if(m_baroReadings.GetCount() >=2)
        {
            wxDateTime oldest(m_baroReadings[0].GetTimeStamp());
            wxDateTime newest(m_baroReadings.Last().GetTimeStamp());
            wxTimeSpan timeSpan = newest - oldest;

            press_trend = barometric_change(m_baroReadings[0].GetReading(), 
                                                m_baroReadings.Last().GetReading());
            //wxLogVerbose(wxString::Format(wxT("press_trend %d %lf %lf"), press_trend, 
            //       m_baroReadings.Last().GetReading(), m_baroReadings[0].GetReading()));
            if(timeSpan.GetHours() >= 3)
            {
                m_baroReadings.RemoveAt(0);
            }
            else
            {
                //wxLogVerbose(wxString::Format(wxT("need moe time %d"), 
                //                                        (int)timeSpan.GetHours()));
            }
        }
        else
        {
//            wxLogVerbose(wxString::Format(wxT("baroReadings count %d"), 
//                                                        m_baroReadings.GetCount()));
        }
    }
    return;
}


int PressureTendency::GetPressureTendency(void)
{
    return press_trend;
}

#if 1
// Custom application traits class which we use to override the default log
// target creation
class MyAppTraits : public wxGUIAppTraits
{
public:
    virtual wxLog *CreateLogTarget();
};
#endif
// ----------------------------------------------------------------------------
// connection class
// ----------------------------------------------------------------------------

class OwwlConnection
{
public:
    time_t GetDataTime(void){ return m_dataTime;}
    unsigned int GetInterval(void){ return m_interval;}
private:
    time_t m_dataTime;
    unsigned int m_interval;
};


#if 1
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
                            wxOK|wxCANCEL|wxCANCEL_DEFAULT|style);
        dlg.SetOKCancelLabels(wxID_COPY, wxID_OK);
        dlg.SetExtendedMessage("Note that this is a custom log dialog.");
        dlg.ShowModal();
    }
};

wxLog *MyAppTraits::CreateLogTarget()
{
    return new MyLogGui;
}
#endif


//-----------------------------------------------------------------------------
// MyApp
//-----------------------------------------------------------------------------

class MyApp: public wxApp
{
public:
    virtual bool OnInit();
    virtual int  OnExit();
#if 1
    virtual void OnInitCmdLine(wxCmdLineParser& parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
    wxString GetCmdLine();
#endif
    void LogPlatform();

protected:
    virtual wxAppTraits *CreateTraits() { return new MyAppTraits; }
};

#if 1
static const wxCmdLineEntryDesc g_cmdLineDesc [] =
{
     { wxCMD_LINE_SWITCH, "h", "help", 
                           "displays help on the command line parameters",
                           wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
     { wxCMD_LINE_SWITCH, "t", "test", "test switch",
                           wxCMD_LINE_VAL_NONE, wxCMD_LINE_PARAM_OPTIONAL
                               /*wxCMD_LINE_OPTION_MANDATORY*/ },
     { wxCMD_LINE_SWITCH, "s", "silent", "disables the GUI" },
     { wxCMD_LINE_NONE }
};
#endif

wxString default_unit_names[] = {"<Metric>", "<Imperial>", "<Alt1>", "<Alt2>"};
int unit_choices[OWWL_UNIT_CLASS_LIMIT];


class MyAuxilliaryFrame;
class MyCanvas;


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


#if 0
//-----------------------------------------------------------------------------
// Class wxShadowDC - probably not the right way to do this...
//-----------------------------------------------------------------------------
class wxShadowDC : public wxClientDC
{
public:
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

    //wxShadowDC(const wxDC& dc) : wxDC(dc) {};
    wxShadowDC(wxWindow* window)/* : wxClientDC(window) {}*/;

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
        //wxFont f;
        //f.GetPointSize();
        switch(m_shadowOffsetMode)
        {
            case wxShadowPixels:
                shadowPt.x += m_shadowOffset;
                shadowPt.y += m_shadowOffset;
                break;
            case wxShadowPercent:
                {
                    int offsetPxls = ((float)m_shadowOffset * 
                                      (float)GetCharHeight() / 100.0);
                    offsetPxls = (0==offsetPxls)?1:offsetPxls;
                    shadowPt.x += offsetPxls;
                    shadowPt.y += offsetPxls;
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


wxShadowDC::wxShadowDC(wxWindow* window) : wxClientDC(window) 
{
    //m_shadowColour = wxBLACK;
    m_shadowOffset = 3;
    m_shadowOffsetDir = wxDownAndRight;
    m_shadowOffsetMode = wxShadowPixels;
}

#endif


//-----------------------------------------------------------------------------
// MyCanvas
//-----------------------------------------------------------------------------
class MyCanvas: public wxPanel
{
public:
    MyCanvas( wxWindow *parent, wxWindowID, const wxPoint &pos, 
              const wxSize &size );
    ~MyCanvas();

    //wxShadowDC *shadowDC;

    void OnPaint( wxPaintEvent &event );
    void CreateAntiAliasedBitmap();
    void DrawText(wxPaintDC * d, wxString str, wxColor fore, wxColor shadow, wxPoint pt );

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
    unsigned long GetInterval(void);
private:
    unsigned long m_interval;
    unsigned int m_pollInterval;
    MyFrame * m_frame;
};


// ----------------------------------------------------------------------------
// MyFrame
// ----------------------------------------------------------------------------
class MyFrame: public wxFrame
{
    OwwlReaderTimer * m_readerTimer;

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
    void OnQuit( wxCommandEvent &event );

    void SetLatLongStatus();
    void SetTitleBar();
    void SetAuxFrameReady(bool f = true) { m_auxFrameReady = f; }

    owwl_conn* ServerReconnect() 
    {
        if (NULL == this) return NULL;
        ServerDisconnect();
        if (NULL == this->m_connection) //actually should be assert()
        {
            InitServerConnection();
        }
        return m_connection;
    }

    void ServerDisconnect() 
    {
        if (NULL == this) return;
        if (NULL == this->m_connection) return;
        owwl_free(m_connection);
        sock_close(m_socket);
        m_socket = -1;
        g_connection = m_connection = NULL;
        return;
    }

    float GetOwwlLatitude(void)
    {
        if (NULL == this) return -0.0;
        if (NULL == this->m_connection) return -0.0;
        return this->m_connection->latitude;
    }

    float GetOwwlLongitude(void)
    {
        if (NULL == this) return -0.0;
        if (NULL == this->m_connection) return -0.0;
        return this->m_connection->longitude;
    }

    int GetOwwlInterval(void)
    {
        if (NULL == this) return -0;
        if (NULL == this->m_connection) return -0;
        return this->m_connection->interval;
    }

    time_t GetOwwlDataTime(void)
    {
        if (NULL == this) return 0;
        if (NULL == this->m_connection) return 0;
        return this->m_connection->data_time
              +this->m_connection->data_time_usec;
    }

    owwl_conn* GetOwwlConnection(void)
    {
        if (NULL == this) return NULL;
        if (NULL == this->m_connection) return NULL;
        return this->m_connection;
    }

    SOCKET GetOwwlSocket(void)
    {
        if (NULL == this) return -1;
        return this->m_socket;
    }

    void SetOwwlSocket(SOCKET s = -1)
    {
        if (NULL == this) return;
        this->m_socket = s;
    }

    owwl_buffer buff;

    wxMenu        *menuImage;
    wxMenu        *subMenu;
    MyCanvas      *m_canvas;
    wxStatusBar   *m_statusbar;
    wxString       m_hostname;
    int            m_port;
    unsigned int   m_pollInterval;
    SOCKET         m_socket;
    owwl_conn     *m_connection;
    wxString       m_mapurl;
    bool           m_launchAtStart;
    int            m_units;
    bool           m_animateDisplay;
    int            m_browser;
    bool           m_restoreAuxFrame;
    int            m_fontSz;
    int            m_windChillAlgor;
    PressureTendency  m_pressTend;
    wxConfigBase  *m_config;

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

    bool m_auxFrameReady;

    DECLARE_DYNAMIC_CLASS(MyFrame)
    DECLARE_EVENT_TABLE()
}; //class MyFrame


//-----------------------------------------------------------------------------
// MyAuxilliaryFrame
//-----------------------------------------------------------------------------
class MyAuxilliaryFrame : public wxFrame
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

    wxGrid *  m_grid;
    MyFrame * m_mainFrame;

private:
    owwl_conn * m_connection;
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
        if (!wxFrame::Create(parent, ID_AUX_FRAME, desc, 
                    wxDefaultPosition, wxSize(10,10),
                    wxDEFAULT_DIALOG_STYLE))
        {
            return false;
        }

        m_updateGridUnits = false;
        m_canvas = canvas;
        m_mainFrame = canvas->m_frame;
        m_connection = canvas->m_frame->m_connection;

        m_grid = new wxGrid(this, wxID_ANY, wxPoint(0,0), wxDefaultSize);
        m_grid->EnableEditing(false);
        m_grid->EnableDragRowSize(false);
        m_grid->EnableDragColSize(false);
        m_grid->CreateGrid(0, 4);
        m_grid->SetColLabelValue(gridColName, "  Name  ");
        m_grid->SetColLabelValue(gridColData, "  Data  ");
        m_grid->SetColLabelValue(gridColValue, "  Value ");
        m_grid->SetColLabelValue(gridColUnit, "  Unit  ");
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
            gSz.IncBy(0, bSz.GetHeight()*1.5);
            SetClientSize(gSz);
        }

        //wxTopLevelWindow.GetPosition();
    	wxPoint pos;
		pos.x = m_mainFrame->m_config->Read(_T("/AuxFrame/x"), -1);
    	pos.y = m_mainFrame->m_config->Read(_T("/AuxFrame/y"), -1);
        wxLogVerbose(wxString::Format(wxT("position: %d %d"), pos.x, pos.y));
        if ( -1 == pos.x )
		{
			pos = parent->GetPosition();
			pos.x += 30;
			pos.y += 30;
        	wxLogVerbose(wxString::Format(wxT("position: %d %d"), pos.x, pos.y));
		}
        wxLogVerbose(wxString::Format(wxT("position: %d %d"), pos.x, pos.y));
        SetPosition(pos);

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
    owwl_conn *conn = m_connection;
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
                    char namebuff[128];
                    int length;
                    int arg = 0;

                    while (arg >= 0)
                    {
                        namebuff[0] = '\0';
                        m_grid->AppendRows();
                        m_grid->SetCellValue(cntr, 0, owwl_name(&(data[i]), namebuff, 
                                                128, &length, 0));
                        m_grid->SetCellValue(cntr, 1, owwl_arg_stem(
                                    (owwl_device_type_enum)data[i].device_type,
                                    data[i].device_subtype, arg));
                        cntr++;
                        arg = owwl_next_arg(&(data[i]), arg);
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
    owwl_conn *conn = m_connection;
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
                        int unit_class, unit = OwwlUnit_Metric;
                        unit_class = owwl_unit_class(data, arg) ;

                        if((unit_class>=0) && (unit_class<OWWL_UNIT_CLASS_LIMIT))
                            unit = unit_choices[unit_class];

                        m_grid->SetCellValue(cntr, 3, owwl_unit_name(&(data[i]), unit, arg));
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
                        int unit_class, unit = OwwlUnit_Metric;
                        unit_class = owwl_unit_class(data, arg);

                        if ((unit_class>=0) && (unit_class<OWWL_UNIT_CLASS_LIMIT))
                            unit = unit_choices[unit_class] ;

                        wxString old_val = m_grid->GetCellValue(cntr, 2);
                        wxString new_val = (data[i]).str(&(data[i]), linebuf, 128,
                                                           unit, -1, arg);
                        if(false == old_val.IsSameAs(new_val))
                        {
                            m_grid->SetCellTextColour(cntr, 2, wxT("RED"));
                        }
                        else
                        {
                            m_grid->SetCellTextColour(cntr, 2,
                                              m_grid->GetDefaultCellTextColour());
                        }
                        m_grid->SetCellValue(cntr, 2, new_val);
                        cntr++;
                        arg = owwl_next_arg(&(data[i]), arg);
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



// ============================================================================
// implementations
// ============================================================================

//-----------------------------------------------------------------------------
// MyAuxilliaryFrame
//-----------------------------------------------------------------------------
BEGIN_EVENT_TABLE(MyAuxilliaryFrame, wxFrame)
    EVT_PAINT(MyAuxilliaryFrame::OnPaint)
#if 0
    EVT_ERASE_BACKGROUND(MyAuxilliaryFrame::OnEraseBackground)
    EVT_MENU(wxID_SAVE, MyAuxilliaryFrame::OnSave)
#endif
END_EVENT_TABLE()

#if 0
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
        arg = owwl_next_arg(data, arg);
    }
  }

  return 0 ;
}
#endif

class Coordinate
{
public:
   Coordinate(double d);

   double getDec(void);
   int getDeg(void) { return m_deg; };
   int getMin(void) { return m_min;} ;
   int getSec(void) { return m_sec; };
   char getDir(void) { return m_dir; };
   double getRad(void) { return m_rad; };

private:
   double m_dec;
   int m_deg;
   int m_min;
   int m_sec;
   char m_dir;
   double m_rad;
};

Coordinate::Coordinate(double val)
{
    m_dec = val;
    m_dir = (0>m_dec)?-1:1;
    m_deg = m_dec * (double)m_dir;
    m_deg = (int)floor((double)m_deg);
    double minsec = ((m_dec * m_dir) - (double)m_deg) * 60.0;
    m_min = floor(minsec);
    m_sec = lround((minsec - (double)m_min) * 60.0);
    m_rad = -0.0;
    return;
}


//-----------------------------------------------------------------------------
// MyFrame
//-----------------------------------------------------------------------------
void MyFrame::changeUnits(int units)
{
    int i;
    for (i=0; i<OWWL_UNIT_CLASS_LIMIT; ++i)
    {
        unit_choices[i] = units;
    }

    if(NULL != subMenu)
    {
        subMenu->Check(Menu_SubMenu_Radio0,(m_units==OwwlUnit_Metric)?true:false);
        subMenu->Check(Menu_SubMenu_Radio1,(m_units==OwwlUnit_Imperial)?true:false);
        subMenu->Check(Menu_SubMenu_Radio2,(m_units==OwwlUnit_Alt1)?true:false);
        subMenu->Check(Menu_SubMenu_Radio3,(m_units==OwwlUnit_Alt2)?true:false);
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
    int x, y;
    GetPosition(&x, &y);
    m_config->Write(_T("/MainFrame/x"), (long) x);
    m_config->Write(_T("/MainFrame/y"), (long) y);

	if (NULL != m_canvas->m_auxilliaryFrame)
	{
		m_canvas->m_auxilliaryFrame->GetPosition(&x, &y);
		m_config->Write(_T("/AuxFrame/x"), (long) x);
		m_config->Write(_T("/AuxFrame/y"), (long) y);
	}

} //MyFrame d-tor



MyFrame::MyFrame()
    : wxFrame( (wxFrame *)NULL, ID_MAIN_FRAME, wxT("oww-wxgui"), 
                wxPoint(20, 20), 
#ifdef __WXGTK__
                wxSize(474, 441+40),
#else
                wxSize(474, 441),
#endif
                wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER|wxMAXIMIZE_BOX)
              )
{
    m_auxFrameReady = false;
    menuImage = NULL;
    subMenu = NULL;
    m_connection = NULL;
    m_canvas = NULL;
    m_statusbar = NULL;
    m_hostname = wxString(wxT("localhost"));
    m_port = 8899;
    m_pollInterval = 10;
    m_socket = -1;
    m_units = OwwlUnit_Imperial;
#if 0    
    buff = Owwl_Buffer_Init;
#else
    memset(&buff, 0, sizeof(owwl_buffer));
#endif
    m_browser = 0;
    m_mapurl = wxEmptyString;
    m_restoreAuxFrame = false;
    m_windChillAlgor = 1;
    m_fontSz = 14;

    //wxLogVerbose(wxString::Format(wxT("Welcome to %s"), wxT("oww-wxgui")));

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
        //wxLogVerbose(wxT("The Winsock dll not found!"));
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
        //wxLogVerbose(wxString::Format(wxT("Winsock dll is version %u.%u"),
        //                   LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion)));
        WSACleanup();
    }
#endif

    SetIcon(wxICON(oww));
    wxStandardPaths::Get().SetInstallPrefix("/usr/local");
    m_config = wxConfigBase::Get(_T("oww-wxgui"));
    m_config->Read(_T("server"), &m_hostname, wxT("localhost"));
    m_port = m_config->ReadLong(_T("port"), m_port);
    m_pollInterval = m_config->ReadLong(_T("pollInterval"), m_pollInterval);
    m_units = m_config->Read(_T("units"), OwwlUnit_Imperial);
    changeUnits(m_units);
    m_browser = m_config->Read(_T("browser"), (long int)0);
    m_config->Read(_T("mapurl"), &m_mapurl, 
            wxT("http://www.openstreetmap.org/?lat=%f&lon=%f&zoom=15&layers=M"));
    m_config->Read(_T("launchStStart"), &m_launchAtStart);
    m_config->Read(_T("animateDisplay"), &m_animateDisplay);
    m_windChillAlgor = m_config->Read(_T("windchillalgorithm"), m_windChillAlgor);
    m_fontSz = m_config->Read(_T("fontSz"), m_fontSz);

    wxMenuBar *menu_bar = new wxMenuBar();
    menuImage = new wxMenu;
    menuImage->Append(ID_AUXILLIARY, wxT("Auxilary"), wxT("See other device values"));
#ifdef __WXMOTIF__
    // Motif doesn't do submenus, soooo....
    menuImage->Append(ID_TOGGLEMENU, wxT("Toggle Units"), wxT("Swap Units"));
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
    menuImage->Append(ID_MAP, wxT("Map"), wxT("Map this station"));
    menuImage->Append(ID_MESSAGES, wxT("Messages"), wxT("Show Messages"));
    menuImage->AppendSeparator();
    menuImage->Append(DIALOGS_PROPERTY_SHEET, wxT("Setup"), wxT("Edit Preferences"));
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

    // restore frame position
    int x = m_config->Read(_T("/MainFrame/x"), 50);
    int y = m_config->Read(_T("/MainFrame/y"), 50);
    int w, h;
    GetClientSize(&w, &h);
    w = m_config->Read(_T("/MainFrame/w"), w);
    h = m_config->Read(_T("/MainFrame/h"), h);
    if(0 > x || 0 > y) // if upper left coner is off screen, move to 10,10
    {
        x = y = 10;
    }
    Move(x, y);

    m_canvas = new MyCanvas( this, wxID_ANY, wxPoint(0,0), wxSize(w,h) );
    Show();

    m_canvas->m_renderTimer = new RenderTimer(m_canvas);
    m_canvas->m_renderTimer->start();

    m_readerTimer = new OwwlReaderTimer(this, m_pollInterval);
    m_readerTimer->start();

    if(0 == InitServerConnection())
    {
        SetTitleBar();
    }
    
    if(true == m_restoreAuxFrame)
    {
        m_canvas->m_auxilliaryFrame = new MyAuxilliaryFrame(this, m_canvas, 
                                                          wxT("Auxilliary Data"));
        m_canvas->m_auxilliaryFrame->Show();
    }
    return;
} //MyFrame c-tor


int MyFrame::InitServerConnection(void)
{
    int retval = 0;
    struct hostent  *host = NULL;
#ifndef __WXMSW__
    struct sockaddr_un addr_un;
#endif
    struct sockaddr_in addr_in;
    int addr_len;
    struct sockaddr *address = NULL;

    //wxLogVerbose(wxString::Format(wxT("Connecting to: %s"), m_hostname.c_str()));

#ifndef __WXMSW__
    if (m_hostname.c_str()[0] == '/') /* AF_LOCAL */
    {
        memset(&addr_un, 0, sizeof(addr_un)) ;
        addr_un.sun_family = AF_LOCAL ;
        strcpy(addr_un.sun_path, m_hostname) ;
        address = (struct sockaddr *) &addr_un ;
        addr_len = sizeof(addr_un.sun_family) + strlen(m_hostname) ;
    }
    else
#endif
    {
        memset(&addr_in, 0, sizeof(struct sockaddr_in));
        bool ipnumaddr = m_hostname.Matches("??.??.??.??");
        if(false == ipnumaddr) /* ip addr is num */
        {
            host = gethostbyname(m_hostname.c_str());
        }
        else /* ip addr is name */
        {
            addr_in.sin_addr.s_addr = inet_addr(m_hostname.c_str());
            host = gethostbyaddr((const char *)&addr_in, 
                                        sizeof(struct sockaddr_in), AF_INET);
        }

        if(NULL != host)
        {
            addr_in.sin_family = AF_INET;
            addr_in.sin_port   = htons(m_port);
            memcpy(&addr_in.sin_addr, host->h_addr_list[0], 
                                                     sizeof(addr_in.sin_addr));
            address = (struct sockaddr *) &addr_in;
            addr_len = sizeof(addr_in);
        }
        else
        {
            //wxLogVerbose(wxT("Unable to resolve host %s error:%s"), 
            //                        m_hostname.c_str(), strerror(sock_error));
            retval = -1;
        }
    } // local port vs address

    // Got a valid address and addr_len (required below here) so lets move on
    if(NULL != address) 
    {
        m_socket = socket(address->sa_family, SOCK_STREAM, 0);
        if(m_socket != -1)
        {
            g_connection = m_connection = owwl_new_conn(m_socket, NULL);
            if (connect(m_socket, address, addr_len) == 0)
            {
                /* Mark the socket as non-blocking */
#ifndef __WXMSW__
                unsigned long mode = 1L;
#else
                u_long mode = 1;
#endif
                IOCTL(m_socket, FIONBIO, &mode);
            }
            else
            {
                //wxLogVerbose(wxT("Connect failed, error: %s"),
                //                                        strerror(sock_error));
                ServerDisconnect();
                retval = -1;
            } //connect()
        }
        else
        {
            //wxLogVerbose(wxT("Open socket failed, error: %s"),
            //                                            strerror(sock_error));
            ServerDisconnect();
            retval = -1;
        } //socket()
    }//host==NULL

    return retval;
} //MyFrame::InitServerConnection

void MyFrame::SetLatLongStatus()
{
    if (NULL == this) return;
    if (NULL == this->m_statusbar) return;
    if (NULL == this->m_connection) return;

    float lat = GetOwwlLatitude();
    float lon = GetOwwlLongitude();
    Coordinate Lat = Coordinate(lat);
    Coordinate Lon = Coordinate(lon);
    wxString strLatLon= wxString::Format(wxT("%d\260%d\"%d\'%c/%d\260%d\"%d\'%c"),
             Lat.getDeg(), Lat.getMin(), Lat.getSec(), (0>Lat.getDir())?'S':'N',
             Lon.getDeg(), Lon.getMin(), Lon.getSec(), (0>Lon.getDir())?'W':'E');
    SetStatusText(strLatLon);
    //wxLogVerbose(strLatLon);

    return;
} //MyFrame::SetLatLongStatus

void MyFrame::SetTitleBar()
{
    if (NULL == this) return;

    wxString titleStr = GetTitle();
    if (true == titleStr.IsEmpty() || true == titleStr.IsNull() || 
                                               NULL == this->m_connection) 
    {
        //wxLogVerbose(wxT("SetTitle"));
        SetTitle(wxT("oww"));
    }
    else
    {
        //can't seem to do titleNew.Format(wxT("oww://%s:%d"), 
        //                 m_hostname.c_str(), (int)m_port); for some reason...
        wxString titleNew = wxString::Format(wxT("oww://%s:%d"), 
                                                m_hostname.c_str(), (int)m_port);
        //wxLogVerbose(titleNew);
        SetTitle(titleNew);
    }

    return;
}

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

    ServerDisconnect();
    /*owwl_free(m_connection);
    sock_close(m_socket);
    m_socket = -1;
    g_connection = m_connection = NULL;
    */
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
    wxDateTime dateTime = wxDateTime::Now();
    wxString buildStr = dateTime.FormatISOCombined();
    wxAboutDialogInfo info;
    //info.SetIcon();
    info.SetName(_("oww-wxgui"));
    info.SetDescription(_("One wire Weather GUI"));
    info.AddDeveloper("Mark Clayton");
    info.AddDeveloper("\nDr. Simon Melhuish, author of oww - Thank You!");
    info.AddDeveloper("\nThe wxWidgets team! - Thank You!");
    info.SetWebSite(_("www.mark-clayton.com/projects.html#oww-wxgui"), 
                                                        _("oww-wxgui website"));
    info.SetVersion(g_VersionStr + "-" + buildStr);
    info.SetCopyright(_T("(C)2012 Mark Clayton mark_clayton@users.sourceforge.net"));

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
#if 0
        wxPaintDC dc(this);
        wxScopedPtr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));

        gc->SetFont(*wxNORMAL_FONT, *wxBLACK);
        gc->DrawText("Text Here", 0, 90/2);

        wxGraphicsFont gf = gc->CreateFont(wxNORMAL_FONT->GetPixelSize().y, "");
        gc->SetFont(gf);
        gc->DrawText("More Text", 0, (3*90)/2);
#endif
    }

    wxDECLARE_NO_COPY_CLASS(MyDevicesFrame);
}; //class MyDeviceFrame



void MyFrame::OnAuxilliary(wxCommandEvent &WXUNUSED(event))
{
    if(true == m_auxFrameReady)
    {
        m_restoreAuxFrame = true;
        m_canvas->m_auxilliaryFrame = new MyAuxilliaryFrame(this, m_canvas,
                                                            wxT("Auxilliary Data"));
        m_canvas->m_auxilliaryFrame->Show();
    }
} //MyFrame::OnAuxilliary



void MyFrame::OnMessages( wxCommandEvent &WXUNUSED(event) )
{
    bool verb = !m_logWindow->GetVerbose();
    //wxLogVerbose(wxT("SetVerbose=%d"), verb);
    m_logWindow->SetVerbose(verb);
    //m_logWindow->Show();
} //MyFrame::OnMessages



void MyFrame::OnMap( wxCommandEvent &WXUNUSED(event) )
{
    float latitude = GetOwwlLatitude();
    float longitude = GetOwwlLongitude();
    //wxLogVerbose(wxT("OnMap Latitude=%3.4f"), latitude);
    //wxLogVerbose(wxT("OnMap Longitude=%3.4f"), longitude);

    //char command[2048];
    //sprintf(command, "open /Applications/Safari.app/Contents/MacOS/Safari " + 
    //                         m_mapurl, latitude, longitude);
    //wxArrayString output;
    wxString url = wxString::Format(m_mapurl, latitude, longitude);
    wxLaunchDefaultBrowser(url);
    //wxExecute(wxString(command), output);
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
            m_config->Write(_T("server"), m_hostname);
            m_config->Write(_T("port"), m_port);
            m_config->Write(_T("pollInterval"), m_pollInterval);
            m_config->Write(_T("launchStStart"), m_launchAtStart);
            m_config->Write(_T("units"), m_units);
            changeUnits(m_units);
            m_config->Write(_T("animateDisplay"), m_animateDisplay);
            m_config->Write(_T("restoreAuxFrame"), m_restoreAuxFrame);
            m_config->Write(_T("browser"), m_browser);
            m_config->Write(_T("mapurl"), m_mapurl);
            m_config->Write(_T("windchillalgorithm"), m_windChillAlgor);
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
    serverText = new wxTextCtrl(panel, ID_SERVER_TEXT, wxEmptyString, 
                                    wxDefaultPosition, wxSize(300, -1), wxTE_LEFT);
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
    itemSizer->Add(new wxStaticText(panel, wxID_STATIC, _("Port:")), 0, 
                                wxALIGN_CENTER_VERTICAL, 5);
    itemSizer->Add(portSpin, 0, wxALL|wxALIGN_CENTER_VERTICAL, 2);
    itemSizer->Add(new wxStaticText(panel, wxID_STATIC, _("Poll Interval (sec):")),
                                0, wxALIGN_CENTER_VERTICAL, 5);
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

    itemSizer->Add(new wxStaticText(panel, wxID_ANY, _("Units:")), 0,
                                                    wxALIGN_CENTER_VERTICAL, 5);
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
    restoreAuxFrame = new wxCheckBox(panel, ID_RESTOREAUX_CHECK,
                                _("Restore Auxilliary Window"),
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

    wxStaticBox* staticBox3 = new wxStaticBox(panel, wxID_ANY, 
                                                        _("Browser Options:"));
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
    urlCmdText = new wxTextCtrl(panel, ID_URLCMD_TEXT, wxEmptyString, 
                        wxDefaultPosition, wxSize(400, -1), wxTE_LEFT);

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

#if WXOWW_LOGWINDOW
    m_logWindow = new wxLogWindow(NULL, wxT("Log"));
    wxFrame *pLogFrame = m_logWindow->GetFrame();
    pLogFrame->SetWindowStyle(wxDEFAULT_FRAME_STYLE);
    pLogFrame->SetSize(wxRect(0,50,500,200));
    m_logWindow->SetVerbose(false);
    wxLog::SetActiveTarget(m_logWindow);
    m_logWindow->Show();
#else
    wxLog::SetVerbose(true);
    FILE *logFile;
#if __WXOSX_COCOA__
    logFile = fopen("/Users/clayton/oww-wxgui.log","w");
#elif __WXGTK__
    logFile = fopen("./oww-wxgui.log","w");
#endif
    //logFile = fopen("/Users/mark/Projects/oww-wxgui/oww-wxgui.log","w");
    wxLogStderr *mStandardLog = new wxLogStderr(logFile);
    wxLog::SetActiveTarget(mStandardLog);
#endif

    wxInitAllImageHandlers();

    wxFrame *frame = new MyFrame();
    frame->Show( true );

    wxString appNameStr = GetAppName();
    LogPlatform();

    return true;
} //MyApp:OnInit


int MyApp::OnExit()
{
#if WXOWW_LOGWINDOW
    delete m_logWindow;
#endif

    // clean up: Set() returns the active config object as Get() does, but unlike
    // Get() it doesn't try to create one if there is none (definitely not what
    // we want here!)
    delete wxConfigBase::Set((wxConfigBase *) NULL);
    return 0;
}



#if 0
wxString MyApp::GetCmdLine()
{
    wxLogVerbose(wxT("cmdln: %s"), MyApp::GetCmdLine().c_str());
    wxString s;
    for(int i=1; i<argc; ++i)
    {
        s.Append(argv[i]);
        s.Append(wxT(" "));
    }
    return s;
} // MyApp::GetCmdLine
#endif
#if 1
void MyApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    parser.SetDesc (g_cmdLineDesc);
    // must refuse '/' as parameter starter or cannot use "/path" style paths
    parser.SetSwitchChars (wxT("-"));
} //MyApp::OnInitCmdLine


bool MyApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    //wxLogVerbose(wxT("cmdln: %s"), MyApp::GetCmdLine().c_str());
    bool help_mode = parser.Found(wxT("h"));
    bool test_mode = parser.Found(wxT("t"));
    bool silent_mode = parser.Found(wxT("s"));

    // to get at your unnamed parameters use
//    wxArrayString files;
//    for (unsigned int i = 0; i < parser.GetParamCount(); i++)
//    {
//            files.Add(parser.GetParam(i));
//}

    // and other command line parameters

    // then do what you need with them.
 
    return true;
} //MyApp::OnCmdLineParsed
#endif

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
    wxLogVerbose(wxT(" * OS Version: %d.%d"), p.GetOSMajorVersion(), 
                                                            p.GetOSMinorVersion());
    wxLogVerbose(wxT(" * Toolkit Version: %d.%d"), p.GetToolkitMajorVersion(), 
                                                        p.GetToolkitMinorVersion());
    wxLogVerbose(wxT(" * Architecture: %s"), p.GetArchName().c_str());
    wxLogVerbose(wxT(" * Endianness: %s"), p.GetEndiannessName().c_str());
    wxLogVerbose(wxT(" * WX ID: %s"), p.GetPortIdName().c_str());
    wxLogVerbose(wxT(" * WX Version: %d.%d.%d.%d"), wxMAJOR_VERSION, 
                        wxMINOR_VERSION, wxRELEASE_NUMBER, wxSUBRELEASE_NUMBER);
    
    return;
} //MyApp::LogPlatform



OwwlReaderTimer::OwwlReaderTimer(MyFrame* f, unsigned int pollInt) : wxTimer()
{
    m_pollInterval = pollInt * 1000;
    OwwlReaderTimer::m_frame = f;
    return;
} // OwwlReaderTimer::OwwlReaderTimer



unsigned long OwwlReaderTimer::GetInterval(void)
{
    return m_interval;
} // OwwlReaderTimer::GetInterval


void OwwlReaderTimer::Notify()
{
    static time_t last = 0;
    bool doit = true;
    //wxLogVerbose("Readertimer:Notify");
#if 1
    if(NULL != m_frame)
    {
        owwl_conn *connection = m_frame->GetOwwlConnection();
        if(NULL != connection)
        {
            //m_frame->m_pressTend.inHg2PressTend(m_frame->m_connection);

            //wxLogVerbose("interval=%d:", m_frame->m_connection->interval);
            m_frame->SetLatLongStatus();
            m_frame->SetTitleBar();

            while(doit)
            {
                doit = false;
                switch(owwl_read(connection))
                {
                    case Owwl_Read_Error:
                        //wxLogVerbose(wxT("Protocol Error"));
                        // drop thru to disconnect break;
                    case Owwl_Read_Disconnect:
                        //wxLogVerbose(wxT("Server Disconnect"));
                        m_frame->SetStatusText(wxT("Server disconnect"));
                        /* Try to reconnect */
                        /* but for now close up */
                        m_frame->ServerDisconnect();
                        /*sock_close(m_frame->m_socket);
                        m_frame->m_socket = -1;
                        g_connection = m_frame->m_connection = NULL;*/
                        last = time(NULL);
                        break;
                    case Owwl_Read_Again:
                        //wxLogVerbose(wxT("Read Again"));
                        if(-1 == owwl_tx_poll_servers(connection))
                        {
                            //wxLogVerbose(wxT("owwl_tx_poll_servers failed"));
                        }
                        wxMilliSleep(10); /* Sleep for 10 ms */
                        if((last != 0) 
                            && (time(NULL)>(last+connection->interval*2+1)))
                        {
                            //wxLogVerbose(wxT("Timeout"));
                            //check_conn(conn) ;
                            last = time(NULL);
                            m_frame->ServerReconnect();
                        }
                        break;
                    case Owwl_Read_Read_And_Decoded:
                    {
                        wxDateTime dataTime;
                        dataTime = wxDateTime(m_frame->GetOwwlDataTime());
                        //wxLogVerbose(wxT("Read And Decode %s"), 
                        //                                dataTime.FormatTime());
                        if(-1 == owwl_tx_build(connection, OWW_TRX_MSG_WSDATA, 
                                                                &(m_frame->buff)))
                        {
                            //wxLogVerbose(wxT("owwl_tx_build failed"));
                        }
                        if(-1 == owwl_tx(connection, &(m_frame->buff)))
                        {
                            //wxLogVerbose(wxT("owwl_tx failed"));
                        }
                        m_frame->m_pressTend.inHg2PressTend(m_frame->m_connection);
                        last = time(NULL);
                        doit = true;
                        m_frame->SetAuxFrameReady(true);
                        break;
                    }
                    default:
                        wxASSERT(wxT("Read Default"));
                        break;
                } //switch(owwl_read(m_frame->m_connection))
            } //while(do)
        } //if(NULL != m_frame->m_connection)
        else
        {
            //wxLogVerbose("OwwlReaderTimer::Notify Reconnectiong...");
            m_frame->ServerReconnect();
        }
    } //if(NULL != m_frame)
#endif
    return;
} //OwwlReader::Notify

void OwwlReaderTimer::start()
{
    //wxLogVerbose(wxT("Poll Interval %d"), m_pollInterval);
    wxTimer::Start(m_pollInterval);
    return;
} // OwwlReader::start




RenderTimer::RenderTimer(MyCanvas *canvas) : wxTimer()
{
    RenderTimer::m_canvas = canvas;
} // RenderTimer c-tor


void RenderTimer::Notify()
{
    //wxLogVerbose("RenderTimer:Notify");
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

    // Try to find the images in the platform specific location
    // Unix: <prefix>/share/appname where prefix is "/usr/local"
    //       unless changed by ::SetInstallPrefix()
    // Windows: the directory where the executable file is located
    // Mac: appname.app/Contents/Resources bundle subdirectory
    wxString dir = wxStandardPaths::Get().GetResourcesDir() + wxT("/pixmaps/");
    //wxLogVerbose(wxString::Format(wxT("Looking for images in %s"), dir.c_str()));

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
#if 0
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
#endif
     }
    else
    {
        //wxLogVerbose(wxT("Can't find image files!"));
    }

    m_counter = 0;
    return;
} //MyCanvas c-tor

MyCanvas::~MyCanvas()
{
    if(NULL != m_renderTimer)
    {
        m_renderTimer->Stop();
        delete m_renderTimer;
    }
} //MyCanvas d-tor

void MyCanvas::DrawText(wxPaintDC * d, wxString str, wxColor fore, wxColor shadow, wxPoint pt)
{
    wxFont f = wxFont(m_frame->m_fontSz, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, 
                                                        wxFONTWEIGHT_NORMAL);
    wxColor fc = d->GetTextForeground();
    d->SetFont(f);
    d->SetTextForeground( shadow );
    d->DrawText( str, pt.x+2, pt.y+2);
    d->SetTextForeground( fore );
    d->DrawText( str, pt.x, pt.y);

    return;
} //MyCanvas::DrawShadowText


void MyCanvas::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
    //wxLogVerbose(_T("start MyCanvas::OnPaint"));
    wxPaintDC dc( this );
    //shadowDC = new wxShadowDC(this);
    //shadowDC->SetTextShadowColour(wxT("BLACK"));
    owwl_data *od = NULL;
    int unit = OwwlUnit_Metric;

    wxFont f = wxFont(m_frame->m_fontSz, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, 
                                                        wxFONTWEIGHT_BOLD);
    dc.SetFont(f);
    dc.SetTextForeground( wxT("WHITE") );

    if(m_frame)
    {
        //wxString now = wxNow ();
        //m_frame->SetStatusText(now, 1);
        wxDateTime dataTime = wxDateTime(m_frame->GetOwwlDataTime());
        m_frame->SetStatusText(dataTime.FormatTime(), 1);

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

        if(m_frame->m_connection)
        {
            float speed = 0.0;
            int bearing = 0;
            char linebuf[128];

            od = owwl_find(m_frame->m_connection, OwwlDev_Wind, 0, 0);
            if(NULL != od)
            {
                int unit_class = owwl_unit_class(od, 0);
                if ((unit_class >= 0) && (unit_class < OWWL_UNIT_CLASS_LIMIT))
                {
                    unit = unit_choices[unit_class];
                }
                speed =   od->val(od, unit, 0);
                bearing = od->val(od, OwwlUnit_Point16, 2);
                //wxLogVerbose(wxString::Format(wxT("bearing %lf"), bearing ));

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

                //int ordinal = lround(bearing/22.5);
                //wxLogVerbose(wxString::Format(wxT("ordinal %lf"), ordinal ));
                switch( bearing )
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

                // Draw Wind Speed and Wind Gust Speed on canvas
                DrawText(&dc, wxString::Format("%s %s (%s %s)",
                                        od->str(od, linebuf, 128, unit, -1, 0),
                                        owwl_unit_name(od, unit, 0), 
                                        od->str(od, linebuf, 128, unit, -1, 1),
                                        owwl_unit_name(od, unit, 1)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(300, 10));
#if 0
                // Draw Wind Bearing on canvas
                DrawText(&dc, wxString::Format("%s", 
                                od->str(od, linebuf, 128, OwwlUnit_Name, -1, 2)),
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(365, 60));
                DrawText(&dc, wxString::Format("(%s)", 
                                od->str(od, linebuf, 128, OwwlUnit_Point16, -1, 2)),
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(400, 60));
#endif
            }
            else
            {
                //wxLogVerbose("MyCanvas::OnPaint od OwwDev_Wind == NULL");
            }

            od = owwl_find(m_frame->m_connection, OwwlDev_Temperature, 
                                                    OwwlTemp_Thermometer, 0);
            if(NULL != od)
            {
                DrawText(&dc, wxString::Format("%s%s", 
                                    od->str(od, linebuf, 128, unit, -1, 0),
                                    owwl_unit_name(od, unit, 0)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(25, 125));
            }
            else
            {
                //wxLogVerbose("MyCanvas::OnPaint od OwwDev_Temperature == NULL");
            }

/* The equivalent formula in US customary units is:

   Twc = 35.74 + 0.6215Ta - 35.75V^0.16 + 0.4275TaV^0.16

    where Twc is the wind chill index, based on the Fahrenheit scale,
    Ta is the air temperature, measured in °F, and V is the wind speed, 
    in mph. Windchill temperature is defined only for temperatures 
    at or below 10 °C (50 °F) and wind speeds above 4.8 kilometres 
    per hour (3.0 mph). As the air temperature falls, the chilling 
    effect of any wind that is present increases. 
*/
            if(m_frame->m_windChillAlgor)
            {
                float speed = 0.0;
                float temperature = 0.0;
                float windchill = 0.0;

                od = owwl_find(m_frame->m_connection, OwwlDev_Wind, 0, 0);
                if(NULL != od)
                {
                    speed = od->val(od, OwwlUnit_Imperial, 0);
                }

                od = owwl_find(m_frame->m_connection, OwwlDev_Temperature, 
                                                    OwwlTemp_Thermometer, 0);
                if(NULL != od)
                {
                    temperature = od->val(od, OwwlUnit_Imperial, 0);
                }

                if(50.0 > temperature && 3.0 < speed)
                {
                    switch(m_frame->m_windChillAlgor)
                    {
                        case 1:
                            speed = pow(speed, 0.16); 
                            windchill = 35.74 + (0.6215 * temperature)
                                        - (35.75 * speed)
                                        + (0.4275 * temperature * speed);
                            break;
                        default:
                                break;
                    }
                    if(NULL != od)
                    {
                        windchill = convertUnits(windchill, OwwlUnit_Imperial, unit);
                        DrawText(&dc, wxString::Format("wc:%2.1f%s", windchill,
                                        owwl_unit_name(od, unit, 0)), 
                            wxT("RED"), wxT("BLACK"), wxPoint(25, 185));
                    }
                }
            }

            od = owwl_find(m_frame->m_connection, OwwlDev_Humidity, 0, 0);
            if(NULL != od)
            {
                if (rh_png.IsOk())
                {
                    dc.DrawBitmap( rh_png, 300, 180 );
                }
                DrawText(&dc, wxString::Format("rh: %s%s", 
                                        od->str(od, linebuf, 128, unit, -1, 0),
                                        owwl_unit_name(od, unit, 0)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(280, 235));
            }
            else
            {
                //wxLogVerbose("MyCanvas::OnPaint od OwwDev_Humidity == NULL");
            }
            if(1)
            {
				float heatindex = 0.0;
                float relativehumidity = 0.0;
                float temperature = 0.0;

                od = owwl_find(m_frame->m_connection, OwwlDev_Humidity, 0, 0);
                if(NULL != od)
                {
                    relativehumidity = od->val(od, OwwlUnit_Imperial, 0);
                }

                od = owwl_find(m_frame->m_connection, OwwlDev_Temperature, 
                                                    OwwlTemp_Thermometer, 0);
                if(NULL != od)
                {
                    temperature = od->val(od, OwwlUnit_Imperial, 0);
                }

				heatindex = heat_index(temperature, relativehumidity);
				if(temperature < heatindex)
                {
                    DrawText(&dc, wxString::Format("hi:%2.1fF %s", heatindex,
										heat_index_str(heat_index_level(heatindex))),
                            wxT("RED"), wxT("BLACK"), wxPoint(25, 185));
                }
            }

            dc.SetTextForeground( wxT("YELLOW") );
            od = owwl_find(m_frame->m_connection, OwwlDev_Temperature, 
                                                            OwwlTemp_Humidity, 0);
            if(NULL != od)
            {
                DrawText(&dc, wxString::Format("Trh: %s %s", 
                            od->str(od, linebuf, 128, unit, -1, 0),
                            owwl_unit_name(od, unit, 0)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(280, 260));
            }
            else
            {
                //wxLogVerbose("MyCanvas::OnPaint od OwwDev_Temp/Humidity==NULL");
            }

            od = owwl_find(m_frame->m_connection, OwwlDev_Temperature, 
                                                            OwwlTemp_Barometer, 0);
            if(NULL != od)
            {
                DrawText(&dc, wxString::Format("Tb: %s %s", 
                            od->str(od, linebuf, 128, unit, -1, 0),
                            owwl_unit_name(od, unit, 0)), 
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(280, 285));
            }
            else
            {
                //wxLogVerbose("MyCanvas::OnPaint od OwwDev_Temp/Barometer == NULL");
            }

            od = owwl_find(m_frame->m_connection, OwwlDev_Barometer, 0, 0);
            if(NULL != od)
            {
                DrawText(&dc, wxString::Format("BP: %s %s", 
                                        od->str(od, linebuf, 128, unit, 5, 0),
                                        owwl_unit_name(od, unit, 0)),
                        wxT("YELLOW"), wxT("BLACK"), wxPoint(280,310));
                switch(m_frame->m_pressTend.GetPressureTendency())
                {
                    case PressureTendency::RAPIDLY_RISING:
                        DrawText(&dc, wxString::Format("Rapidly Rising"), wxT("RED"),
                                                    wxT("BLACK"), wxPoint(280,335));
                        break;
                    case PressureTendency::QUICKLY_RISING:
                        DrawText(&dc, wxString::Format("Quickly Rising"), wxT("RED"),
                                                    wxT("BLACK"), wxPoint(280,335));
                        break;
                    case PressureTendency::RISING:
                        DrawText(&dc, wxString::Format("Rising"), wxT("YELLOW"),
                                                    wxT("BLACK"), wxPoint(280,335));
                        break;
                    case PressureTendency::SLOWLY_RISING:
                        DrawText(&dc, wxString::Format("Slowly Rising"), wxT("GREEN"),
                                                    wxT("BLACK"), wxPoint(280,335));
                        break;
                    case PressureTendency::STEADY:
                        DrawText(&dc, wxString::Format("Steady"), wxT("GREEN"),
                                                    wxT("BLACK"), wxPoint(280,335));
                        break;
                    case PressureTendency::SLOWLY_FALLING:
                        DrawText(&dc, wxString::Format("Slowly Falling"), wxT("GREEN"),
                                                    wxT("BLACK"), wxPoint(280,335));
                        break;
                    case PressureTendency::FALLING:
                        DrawText(&dc, wxString::Format("Falling"), wxT("YELLOW"), 
                                                    wxT("BLACK"), wxPoint(280,335));
                        break;
                    case PressureTendency::QUICKLY_FALLING:
                        DrawText(&dc, wxString::Format("Quickly Falling"), wxT("GREEN"),
                                                    wxT("BLACK"), wxPoint(280,335));
                        break;
                    case PressureTendency::RAPIDLY_FALLING:
                        DrawText(&dc, wxString::Format("Rapidly Falling"), wxT("RED"),
                                                    wxT("BLACK"), wxPoint(280,335));
                        break;
                    case PressureTendency::ARRAY_ERROR:
                    default:
                        DrawText(&dc, wxString::Format("------n/a------"), wxT("YELLOW"),
                                                    wxT("BLACK"), wxPoint(280,335));
                        break;
                }
            }
            else
            {
                //wxLogVerbose("MyCanvas::OnPaint od OwwDev_Barometer == NULL");
            }

            dc.SetTextForeground( wxT("RED") );
            od = owwl_find(m_frame->m_connection, OwwlDev_Rain, 0, 0);
            if(NULL != od)
            {
                wxDateTime rain_time = wxDateTime(od->device_data.rain.rain_reset_time);
                //wxString rain_count(od->str(od, linebuf, 128, unit, -1, 0));
                //wxString rain_rate(od->str(od, linebuf, 128, unit, -1, 3));
                float  rain_count = od->val(od, unit, 0);
                float  rain_rate = od->val(od, unit, 2);
                DrawText(&dc, wxString::Format("Rain: %1.2f%s since %s (%1.2f%s)", 
                                        rain_count,
                                        owwl_unit_name(od, unit, 0),
                                        rain_time.FormatTime(),
                                        rain_rate,
                                        owwl_unit_name(od, unit, 2)),
                            wxT("YELLOW"), wxT("BLACK"), wxPoint(25, 370));
            }
            else
            {
                //wxLogVerbose("MyCanvas::OnPaint od OwwDev_Rain == NULL");
            }

            wxRectangle area;
            area.x = 300;
            area.y = 50;
            area.width = 100;
            area.height = 100; 
            mainwin_draw_compass_rose(&dc, 1, bearing, area);
            dc.SetBrush( *wxWHITE_BRUSH );
            dc.SetPen( *wxBLACK_PEN );
            //dc.DrawCircle( 325, 120, 25);
            //dc.SetBrush( *wxWHITE_BRUSH );
            //dc.SetPen( *wxRED_PEN );
            //dc.DrawRectangle( 322, 50, 5, 10 );

        }
        else
        {
            //wxLogVerbose("MyCanvas::OnPaint m_conn==NULL");
        }// if(m_connection)

#if 0
        wxScopedPtr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        wxGraphicsFont gf = gc->CreateFont(wxNORMAL_FONT->GetPixelSize().y, wxT("WHITE"));

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
    }// if(m_frame)
    //wxLogVerbose(_T("end MyCanvas::OnPaint"));
} //MyCanvas::OnPaint


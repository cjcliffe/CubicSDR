#pragma once

#include <map>
#include <vector>
#include <sstream>
#include <ostream>
#include <mutex>

#include "DigitalConsoleFrame.h"
#include "ModemDigital.h"

class ModemDigitalOutputConsole;
class DigitalConsole: public DigitalConsoleFrame {
public:
    DigitalConsole( wxWindow* parent, ModemDigitalOutputConsole *doParent );
    ~DigitalConsole();


    void write(std::string outp);
    void write(char outc);
    
private:
    void DoRefresh( wxTimerEvent& event );
    void OnClose( wxCloseEvent& event );
    void OnClear( wxCommandEvent& event );
    
    void OnCopy( wxCommandEvent& event );
    void OnPause( wxCommandEvent& event );

    std::stringstream streamBuf;
    std::mutex stream_busy;
    std::atomic<bool> streamWritten;
    std::atomic<bool> streamPaused;
    ModemDigitalOutputConsole *doParent;
};

class ModemDigitalOutputConsole: public ModemDigitalOutput {
public:
    ModemDigitalOutputConsole();
    ~ModemDigitalOutputConsole();
    
    void setDialog(DigitalConsole *dialog_in);
    DigitalConsole *getDialog();

    void setTitle(std::string title);
    
    void write(std::string outp);
    void write(char outc);
    
    void Show();
    void Hide();
    void Close();

private:
    DigitalConsole *dialog;
    std::stringstream streamBuf;
    std::mutex stream_busy;
    std::atomic<bool> streamWritten;
    std::string dialogTitle;
};


// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

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
    ~DigitalConsole() override;


    void write(const std::string& outp);
    void write(char outc);
    
private:
    void DoRefresh( wxTimerEvent& event ) override;
    void OnClose( wxCloseEvent& event ) override;
    void OnClear( wxCommandEvent& event ) override;
    
    void OnCopy( wxCommandEvent& event ) override;
    void OnPause( wxCommandEvent& event ) override;

    std::stringstream streamBuf;
    std::mutex stream_busy;
    std::atomic<bool> streamWritten;
    std::atomic<bool> streamPaused;
    ModemDigitalOutputConsole *doParent;
};

class ModemDigitalOutputConsole: public ModemDigitalOutput {
public:
    ModemDigitalOutputConsole();
    ~ModemDigitalOutputConsole() override;
    
    void setDialog(DigitalConsole *dialog_in);
    DigitalConsole *getDialog();

    void setTitle(const std::string& title);
    
    void write(std::string outp) override;
    void write(char outc) override;
    
    void Show() override;
    void Hide() override;
    void Close() override;

private:
    DigitalConsole *dialog;
    std::stringstream streamBuf;
    std::mutex stream_busy;
    std::atomic<bool> streamWritten;
    std::string dialogTitle;
};


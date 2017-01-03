// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "DigitalConsole.h"
#include "CubicSDR.h"
#include <iomanip>

DigitalConsole::DigitalConsole( wxWindow* parent, ModemDigitalOutputConsole *doParent ): DigitalConsoleFrame( parent ), doParent(doParent) {
    streamWritten.store(false);
    streamPaused.store(false);
}

DigitalConsole::~DigitalConsole() {
    doParent->setDialog(nullptr);
}

void DigitalConsole::OnClose( wxCloseEvent& event ) {
    doParent->setDialog(nullptr);
}

void DigitalConsole::OnCopy( wxCommandEvent& event ) {
    m_dataView->SelectAll();
    m_dataView->Copy();
}

void DigitalConsole::OnPause( wxCommandEvent& event ) {
    if (streamPaused.load()) {
        m_pauseButton->SetLabel("Stop");
        streamPaused.store(false);
    } else {
        m_pauseButton->SetLabel("Run");
        streamPaused.store(true);
    }
}

void DoRefresh( wxTimerEvent& event ) {
    event.Skip();
}

void DigitalConsole::DoRefresh( wxTimerEvent& event ) {
    if (streamWritten.load()) {
        stream_busy.lock();
        m_dataView->AppendText(streamBuf.str());
        streamBuf.str("");
        streamWritten.store(false);
        stream_busy.unlock();
    }
}

void DigitalConsole::OnClear( wxCommandEvent& event ) {
    m_dataView->Clear();
}

void DigitalConsole::write(std::string outp) {
    if (streamPaused.load()) {
        return;
    }
    stream_busy.lock();
    streamBuf << outp;
    streamWritten.store(true);
    stream_busy.unlock();
}

void DigitalConsole::write(char outc) {
    if (streamPaused.load()) {
        return;
    }
    stream_busy.lock();
    streamBuf << outc;
    streamWritten.store(true);
    stream_busy.unlock();
}


ModemDigitalOutputConsole::ModemDigitalOutputConsole(): ModemDigitalOutput(), dialog(nullptr) {
    streamWritten.store(false);
}

ModemDigitalOutputConsole::~ModemDigitalOutputConsole() {
    
}

void ModemDigitalOutputConsole::setDialog(DigitalConsole *dialog_in) {
    dialog = dialog_in;
    if (dialog && dialogTitle != "") {
        dialog->SetTitle(dialogTitle);
    }
}

DigitalConsole *ModemDigitalOutputConsole::getDialog() {
    return dialog;
}

void ModemDigitalOutputConsole::Show() {
    if (!dialog) {
        return;
    }
    if (!dialog->IsShown()) {
        dialog->Show();
    }
}


void ModemDigitalOutputConsole::Hide() {
    if (!dialog) {
        return;
    }
    if (dialog->IsShown()) {
        dialog->Hide();
    }
}

void ModemDigitalOutputConsole::Close() {
    if (!dialog) {
        return;
    }
    dialog->Hide();
    dialog->Close();
    dialog = nullptr;
}

void ModemDigitalOutputConsole::setTitle(std::string title) {
    if (dialog) {
        dialog->SetTitle(title);
    }
    dialogTitle = title;
}

void ModemDigitalOutputConsole::write(std::string outp) {
    if (!dialog) {
        return;
    }
    dialog->write(outp);
}

void ModemDigitalOutputConsole::write(char outc) {
    if (!dialog) {
        return;
    }
    dialog->write(outc);
}

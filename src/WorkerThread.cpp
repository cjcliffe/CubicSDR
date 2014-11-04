
 
 
   class WorkerThread : public wxThread
    {
    public:
      WorkerThread(SDRThreadQueue* pQueue, int id=0) : m_pQueue(pQueue), m_ID(id) { assert(pQueue); wxThread::Create(); }
 
    private:
      SDRThreadQueue* m_pQueue;
      int m_ID;
 
      virtual wxThread::ExitCode Entry()
      {
        Sleep(1000); // sleep a while to simulate some time-consuming init procedure
        SDRThreadTask::SDR_COMMAND iErr;
        m_pQueue->Report(SDRThreadTask::SDR_THREAD_STARTED, wxEmptyString, m_ID); // tell main thread that worker thread has successfully started
        try { while(true) OnTask(); } // this is the main loop: process tasks until a task handler throws
        catch(SDRThreadTask::SDR_COMMAND& i) { m_pQueue->Report(iErr=i, wxEmptyString, m_ID); } // catch return value from error condition
        return (wxThread::ExitCode)iErr; // and return exit code
      } // virtual wxThread::ExitCode Entry()
 
      virtual void OnTask()
      {
        SDRThreadTask task=m_pQueue->Pop(); // pop a task from the queue. this will block the worker thread if queue is empty
        switch(task.m_cmd)
        {
        case SDRThreadTask::SDR_THREAD_EXIT: // thread should exit
          Sleep(1000); // wait a while
          throw SDRThreadTask::SDR_THREAD_EXIT; // confirm exit command
        case SDRThreadTask::SDR_THREAD_JOB: // process a standard task
          Sleep(2000);
          m_pQueue->Report(SDRThreadTask::SDR_THREAD_JOB, wxString::Format(wxT("Task #%s done."), task.m_Arg.c_str()), m_ID); // report successful completion
          break;
        case SDRThreadTask::SDR_THREAD_JOBERR: // process a task that terminates with an error
          m_pQueue->Report(SDRThreadTask::SDR_THREAD_JOB, wxString::Format(wxT("Task #%s errorneous."), task.m_Arg.c_str()), m_ID);
          Sleep(1000);
          throw SDRThreadTask::SDR_THREAD_EXIT; // report exit of worker thread
          break;
        case SDRThreadTask::SDR_THREAD_NULL: // dummy command
        default: break; // default
        } // switch(task.m_cmd)
      } // virtual void OnTask()
    }; // class WorkerThread : public wxThread
 
 
    // ----------------------------------------------------------------------------
    // main frame
    // ----------------------------------------------------------------------------
    class MyFrame : public wxFrame
    {
      enum { eQUIT=wxID_CLOSE, eSTART_THREAD=wxID_HIGHEST+100 };
      DECLARE_DYNAMIC_CLASS(MyFrame)
    public:
      MyFrame(const wxString& title) : wxFrame(NULL, wxID_ANY, title), m_pQueue(NULL)
      {
        wxMenu *fileMenu=new wxMenu;
        fileMenu->Append(eSTART_THREAD,   _T("&Start a thread\tAlt-S"),     _T("Starts one worker thread"));
        fileMenu->Append(SDRThreadTask::SDR_THREAD_EXIT,     _T("Sto&p a thread\tAlt-P"), _T("Stops one worker thread"));
        fileMenu->Append(SDRThreadTask::SDR_THREAD_JOB,      _T("&Add task to thread\tAlt-A"),  _T("Adds a task to the worker thread"));
        fileMenu->Append(SDRThreadTask::SDR_THREAD_JOBERR,   _T("Add &errorneous task to thread\tAlt-E"), _T("Adds am errorneous task to the worker thread"));
        fileMenu->Append(eQUIT,      _T("E&xit\tAlt-X"),  _T("Exit this program"));
        wxMenuBar *menuBar=new wxMenuBar();
        menuBar->Append(fileMenu, _T("&Options"));
        SetMenuBar(menuBar);
        EnableThreadControls(false); // disable thread controls since worker thread isn't running yet
        fileMenu->Enable(eSTART_THREAD, true); // starting threads should always be possible
        CreateStatusBar();
        SetStatusText(_T("Worker thread sample"));
        m_pQueue=new SDRThreadQueue(this);
      } // MyFrame(const wxString& title)
      ~MyFrame() { delete m_pQueue; }
      void EnableThreadControls(bool bEnable)
      {
        wxMenu* pMenu=GetMenuBar()->GetMenu(0);
        static const int MENUIDS[]={eSTART_THREAD, SDRThreadTask::SDR_THREAD_EXIT, SDRThreadTask::SDR_THREAD_JOB, SDRThreadTask::SDR_THREAD_JOBERR};
        for(int i=0; i<WXSIZEOF(MENUIDS); pMenu->Enable(MENUIDS[i++], bEnable));
      }
      void OnStart(wxCommandEvent& WXUNUSED(event)) // start one worker thread
      {
        int id=m_Threads.empty()?1:m_Threads.back()+1;
        m_Threads.push_back(id);
        WorkerThread* pThread=new WorkerThread(m_pQueue, id); // create a new worker thread, increment thread counter (this implies, thread will start OK)
        pThread->Run();
        SetStatusText(wxString::Format(wxT("[%i]: Thread started."), id));
      }
      void OnStop(wxCommandEvent& WXUNUSED(event)) // stop one worker thread
      {
        if(m_Threads.empty()) { EnableThreadControls(false); Destroy(); return; } // no thread(s) running: frame can be destroyed right away
        m_pQueue->AddTask(SDRThreadTask(SDRThreadTask::SDR_THREAD_EXIT, wxEmptyString), SDRThreadQueue::eHIGHEST); // add SDR_THREAD_EXIT notification with highest priority to bypass other running tasks
        SetStatusText(_T("Stopping thread..."));
      }
      void OnTask(wxCommandEvent& event) // handler for launching a task for worker thread(s)
      {
        int iTask=rand();
        m_pQueue->AddTask(SDRThreadTask((SDRThreadTask::SDR_COMMAND)event.GetId(), wxString::Format(wxT("%u"), iTask)));
        SetStatusText(wxString::Format(wxT("Task #%i started."), iTask)); // just set the status text
      }
      void OnThread(wxCommandEvent& event) // handler for thread notifications
      {
        switch(event.GetId())
        {
        case SDRThreadTask::SDR_THREAD_JOB:
          SetStatusText(wxString::Format(wxT("[%i]: %s"), event.GetInt(), event.GetString().c_str())); // progress display
          break; 
        case SDRThreadTask::SDR_THREAD_EXIT:
          SetStatusText(wxString::Format(wxT("[%i]: Stopped."), event.GetInt()));
          m_Threads.remove(event.GetInt()); // thread has exited: remove thread ID from list
          if(m_Threads.empty()) { EnableThreadControls(false); Destroy(); } // destroy main window if no more threads
          break;
        case SDRThreadTask::SDR_THREAD_STARTED:
          SetStatusText(wxString::Format(wxT("[%i]: Ready."), event.GetInt()));
          EnableThreadControls(true); // at least one thread successfully started: enable controls
          break; 
        default: event.Skip();
        }
      }
      void OnQuit(wxCommandEvent& WXUNUSED(event))
      {
        if(m_Threads.empty()) { Destroy(); return; } // no thread(s) running - exit right away
        for(size_t t=0; t<m_Threads.size(); ++t) m_pQueue->AddTask(SDRThreadTask(SDRThreadTask::SDR_THREAD_EXIT, wxEmptyString), SDRThreadQueue::eHIGHEST); } // send all running threads the "EXIT" signal
      void OnClose(wxCloseEvent& WXUNUSED(event))  { wxCommandEvent e; OnQuit(e); } // just run OnQuit() which will terminate worker threads and destroy the main frame
    private:
      MyFrame() : wxFrame() {}
      SDRThreadQueue* m_pQueue;
      std::list<int> m_Threads;
      DECLARE_EVENT_TABLE()
    }; // class MyFrame : public wxFrame
    IMPLEMENT_DYNAMIC_CLASS(MyFrame, wxFrame)
    BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(SDRThreadTask::SDR_THREAD_JOB, MyFrame::OnTask)
    EVT_MENU(SDRThreadTask::SDR_THREAD_JOBERR, MyFrame::OnTask)
    EVT_MENU(eSTART_THREAD, MyFrame::OnStart)
    EVT_MENU(SDRThreadTask::SDR_THREAD_EXIT, MyFrame::OnStop)
    EVT_COMMAND(wxID_ANY, wxEVT_THREAD, MyFrame::OnThread)
    EVT_MENU(eQUIT, MyFrame::OnQuit)
    EVT_CLOSE(MyFrame::OnClose)
    END_EVENT_TABLE()
 
    // ----------------------------------------------------------------------------
    // the application
    // ----------------------------------------------------------------------------
    class MyApp : public wxApp
    {
    public:
      virtual bool OnInit()
      {
        if(!wxApp::OnInit()) return false;
        MyFrame *frame=new MyFrame(_T("Minimal wxWidgets App"));
        frame->Show(true);
        return true;
      }
    }; // class MyApp : public wxApp
    IMPLEMENT_APP(MyApp)
#include "AudioThread.h"
#include "CubicSDRDefs.h"
#include <vector>

//wxDEFINE_EVENT(wxEVT_COMMAND_AudioThread_INPUT, wxThreadEvent);

AudioThread::AudioThread(AudioThreadQueue* pQueue, int id) :
		wxThread(wxTHREAD_DETACHED), m_pQueue(pQueue), m_ID(id), audio_queue_ptr(
				0) { //, stream(NULL)

}
AudioThread::~AudioThread() {
	ao_close(device);
	ao_shutdown();
}

wxThread::ExitCode AudioThread::Entry() {

	ao_initialize();

	/* -- Setup for default driver -- */

	int default_driver;

	default_driver = ao_default_driver_id();

	memset(&format, 0, sizeof(format));
	format.bits = 16;
	format.channels = 2;
	format.rate = AUDIO_FREQUENCY;
	format.byte_format = AO_FMT_LITTLE;

	/* -- Open driver -- */
	device = ao_open_live(default_driver, &format, NULL /* no options */);
	if (device == NULL) {
		fprintf(stderr, "Error opening device.\n");
		return (wxThread::ExitCode) 1;
	}

	while (!TestDestroy()) {

		if (m_pQueue->stackSize()) {

			while (m_pQueue->stackSize()) {
				AudioThreadTask task = m_pQueue->pop(); // pop a task from the queue. this will block the worker thread if queue is empty
				switch (task.m_cmd) {
				case AudioThreadTask::AUDIO_THREAD_DATA:

					int16_t buf[task.data->data.size()];

					for (int i = 0; i < task.data->data.size(); i++) {
						buf[i] = (int) (task.data->data[i] * 32760.0);
					}

					ao_play(device, (char *) buf,
							task.data->data.size() * sizeof(int16_t));

					delete task.data;
					break;
				}
			}
		} else {
			this->Yield();
			this->Sleep(1);
		}
	}
	std::cout << std::endl << "Audio Thread Done." << std::endl << std::endl;

	return (wxThread::ExitCode) 0;
}


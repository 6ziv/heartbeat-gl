#include "playmidi.h"
#include <string>
#include <sstream>
#include "midi.h"
#include "midifile/src/Binasc.cpp"
#include "midifile/src/Options.cpp"
#include "midifile/src/MidiEvent.cpp"
#include "midifile/src/MidiEventList.cpp"
#include "midifile/src/MidiMessage.cpp"
#include "midifile/src/MidiFile.cpp"

#include <Windows.h>
#pragma comment(lib,"winmm.lib")
DWORD WINAPI PlayMidiProc(LPVOID) {
    std::string str(reinterpret_cast<const char*>(&bin2c_midi_mid[0]), sizeof(bin2c_midi_mid));
    std::istringstream iss(str, std::ios::binary);

    smf::MidiFile midifile;
    midifile.readSmf(iss);
    midifile.doTimeAnalysis();
    //midifile.linkNotePairs();
    //midifile.joinTracks();

    std::vector<std::pair<double, std::vector<uint8_t>>> messages;


    for (size_t i = 0; i < midifile.size(); i++) {
        const auto& track = midifile[i];
        for (size_t eventid = 0; eventid < track.size(); eventid++) {
            const auto& midi_ev = track[eventid];
            if (!midi_ev.isMeta()) {
                messages.emplace_back(1000 * midi_ev.seconds, midi_ev);
            }
        }
    }
    std::sort(messages.begin(), messages.end());



    HMIDIOUT midiout;
    if (MMSYSERR_NOERROR != midiOutOpen(&midiout, 0, NULL, NULL, CALLBACK_NULL))return 1;

    timeBeginPeriod(1);
    while (1) {
        unsigned long long begin_time = GetTickCount64();
        size_t pos = 0;
        while (pos < messages.size()) {
            unsigned long long note_ts = GetTickCount64() - begin_time;
            if (note_ts >= messages[pos].first) {

                MIDIHDR sysex;
                sysex.lpData = (LPSTR)messages[pos].second.data();
                sysex.dwBufferLength = messages[pos].second.size();
                sysex.dwFlags = 0;
                if (MMSYSERR_NOERROR != midiOutPrepareHeader(midiout, &sysex, sizeof(MIDIHDR)))return 1;
                if (MMSYSERR_NOERROR != midiOutLongMsg(midiout, &sysex, sizeof(MIDIHDR)))return 1;

                while (MIDIERR_STILLPLAYING == midiOutUnprepareHeader(midiout, &sysex, sizeof(MIDIHDR)));


                //midiout.sendMessage(messages[pos].second.data(), messages[pos].second.size());
                pos++;
            }
            else {
                if (messages[pos].first - note_ts > 20)Sleep(messages[pos].first - note_ts - 3);
            }
            // do not eat too much CPU.
        }
    }
    timeEndPeriod(1);
}
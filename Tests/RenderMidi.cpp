// Renderiza una secuencia de notas con un preset de RawTek.
//   RawTekRenderMidi <preset.rtkpreset | factory:N> <events.txt> <seconds> <bpm> <out.wav>
// events.txt: una nota por línea -> "inicio_seg nota velocidad(0..1) duracion_seg"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/PluginProcessor.h"
#include <iostream>

using namespace juce;

struct FixedPlayHead : AudioPlayHead
{
    double bpm = 120.0;
    int64 samplePos = 0;
    double sr = 48000.0;
    Optional<PositionInfo> getPosition() const override
    {
        PositionInfo info;
        info.setBpm (bpm);
        info.setIsPlaying (true);
        info.setTimeInSamples (samplePos);
        info.setPpqPosition ((double) samplePos / sr * bpm / 60.0);
        return info;
    }
};

int main (int argc, char** argv)
{
    if (argc < 6) { std::cerr << "uso: RawTekRenderMidi preset events seconds bpm out.wav\n"; return 2; }
    ScopedJuceInitialiser_GUI init;
    const auto cwd = File::getCurrentWorkingDirectory();
    const String presetSpec (argv[1]);
    const File eventsFile = cwd.getChildFile (argv[2]);
    const double seconds = String (argv[3]).getDoubleValue();
    const double bpm = String (argv[4]).getDoubleValue();
    const File outFile = cwd.getChildFile (argv[5]);

    constexpr double sr = 48000.0;
    constexpr int block = 256;

    std::unique_ptr<AudioProcessor> base (createPluginFilter());
    auto& proc = *dynamic_cast<RawTekProcessor*> (base.get());
    FixedPlayHead ph; ph.bpm = bpm; ph.sr = sr;
    proc.setPlayHead (&ph);
    proc.setPlayConfigDetails (0, 2, sr, block);

    if (presetSpec.startsWith ("factory:")) proc.setCurrentProgram (presetSpec.fromFirstOccurrenceOf (":", false, false).getIntValue());
    else if (! proc.loadPresetFile (cwd.getChildFile (presetSpec))) { std::cerr << "no pude cargar " << presetSpec << "\n"; return 1; }
    proc.prepareToPlay (sr, block);

    struct Ev { int sample; bool on; int note; float vel; };
    std::vector<Ev> evs;
    StringArray lines;
    eventsFile.readLines (lines);
    for (auto& l : lines)
    {
        auto t = StringArray::fromTokens (l.trim(), " ", "");
        if (t.size() < 4) continue;
        const int s0 = (int) std::round (t[0].getDoubleValue() * sr);
        const int s1 = (int) std::round ((t[0].getDoubleValue() + t[3].getDoubleValue()) * sr);
        evs.push_back ({ s0, true, t[1].getIntValue(), (float) t[2].getDoubleValue() });
        evs.push_back ({ jmax (s0 + 1, s1), false, t[1].getIntValue(), 0.0f });
    }
    std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) {
        if (a.sample != b.sample) return a.sample < b.sample;
        return ! a.on && b.on;
    });

    const int total = (int) (seconds * sr);
    AudioBuffer<float> out (2, total);
    size_t ei = 0;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        AudioBuffer<float> buf (2, n);
        buf.clear();
        MidiBuffer midi;
        while (ei < evs.size() && evs[ei].sample < pos + n)
        {
            const auto& e = evs[ei++];
            midi.addEvent (e.on ? MidiMessage::noteOn (1, e.note, e.vel) : MidiMessage::noteOff (1, e.note),
                           jmax (0, e.sample - pos));
        }
        ph.samplePos = pos;
        proc.processBlock (buf, midi);
        for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, pos, buf, ch, 0, n);
    }

    // compensar latencia del oversampling
    const int lat = proc.getLatencySamples();
    if (lat > 0)
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = out.getWritePointer (ch);
            std::memmove (d, d + lat, (size_t) (total - lat) * sizeof (float));
            FloatVectorOperations::clear (d + total - lat, lat);
        }

    WavAudioFormat wav;
    outFile.getParentDirectory().createDirectory();
    outFile.deleteFile();
    if (auto stream = outFile.createOutputStream())
        if (std::unique_ptr<AudioFormatWriter> w { wav.createWriterFor (stream.get(), sr, 2, 32, {}, 0) })
        {
            stream.release();
            w->writeFromAudioSampleBuffer (out, 0, total);
        }
    std::cout << "ok " << outFile.getFileName() << " peak " << out.getMagnitude (0, total) << std::endl;
    return 0;
}

// Render offline: presets de fábrica + (opcional) carpeta de .rtkpreset.
// Chequea NaN/inf y niveles, escribe WAVs y una captura del editor.
//   RawTekRenderTest <outDir> [presetDir]
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/PluginProcessor.h"
#include <iostream>

using namespace juce;

namespace
{
constexpr double sr = 48000.0;
constexpr int block = 256;
constexpr double stepLen = 60.0 / 130.0 / 4.0; // 16avos a 130 BPM

struct Hit { int step; std::vector<int> notes; double gate; float vel; };

// Patrón de 32 pasos según la categoría del preset
std::vector<Hit> patternFor (const String& cat)
{
    std::vector<Hit> hits;
    const int bassLine[16] = { 36, 36, 48, 36, 39, 36, 36, 46, 36, 43, 36, 36, 48, 36, 41, 39 };

    if (cat == "Bass" || cat == "Lead" || cat == "Factory")
    {
        const int off = cat == "Lead" ? 24 : 0;
        for (int s = 0; s < 32; ++s)
            hits.push_back ({ s, { bassLine[s % 16] + off }, 0.6, (s % 4 == 2) ? 1.0f : 0.6f });
    }
    else if (cat == "Metal" || cat == "Pluck" || cat == "Perc" || cat == "Groove" || cat == "Hard Groove" || cat == "Groove Perc" || cat == "Old School" || cat == "Wavetable")
    {
        const int steps[] = { 0, 3, 6, 8, 10, 11, 14 };
        const int notes[] = { 48, 48, 55, 51, 48, 58, 53 };
        const int off = (cat == "Perc" || cat == "Groove Perc") ? 0 : 12;
        for (int bar = 0; bar < 2; ++bar)
            for (int i = 0; i < 7; ++i)
                hits.push_back ({ bar * 16 + steps[i], { notes[i] + off }, 0.35, i % 2 == 0 ? 1.0f : 0.7f });
    }
    else if (cat == "Stab" || cat == "Rave")
    {
        for (int s = 2; s < 32; s += 4)
        {
            const int root = (s % 16 < 8) ? 60 : 58;
            hits.push_back ({ s, { root, root + 3, root + 7 }, 0.3, 0.9f });
        }
    }
    else // Drone, FX, otros: notas largas
    {
        hits.push_back ({ 0,  { 48, 55 }, 14.0, 0.9f });
        hits.push_back ({ 16, { 46, 53 }, 14.0, 0.9f });
    }
    return hits;
}

struct Stats { double peak = 0, rms = 0; bool bad = false; };

Stats renderAndWrite (AudioProcessor& proc, const String& category, const File& wavFile)
{
    proc.prepareToPlay (sr, block); // reset de estado
    const int total = (int) (sr * stepLen * 32 + sr * 0.5);
    AudioBuffer<float> out (2, total);
    out.clear();

    struct Ev { int sample; MidiMessage msg; };
    std::vector<Ev> events;
    for (const auto& h : patternFor (category))
    {
        const int on  = (int) std::round (h.step * sr * stepLen);
        const int off = (int) std::round ((h.step + h.gate) * sr * stepLen);
        for (int n : h.notes)
        {
            events.push_back ({ on, MidiMessage::noteOn (1, n, h.vel) });
            events.push_back ({ jmin (off, total - 1), MidiMessage::noteOff (1, n) });
        }
    }
    std::stable_sort (events.begin(), events.end(), [] (const Ev& a, const Ev& b) {
        if (a.sample != b.sample) return a.sample < b.sample;
        return a.msg.isNoteOff() && b.msg.isNoteOn(); // note-off antes que note-on en el mismo sample
    });

    size_t ei = 0;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        AudioBuffer<float> buf (2, n);
        buf.clear();
        MidiBuffer midi;
        while (ei < events.size() && events[ei].sample < pos + n)
        {
            midi.addEvent (events[ei].msg, events[ei].sample - pos);
            ++ei;
        }
        proc.processBlock (buf, midi);
        for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, pos, buf, ch, 0, n);
    }

    Stats st;
    double sumSq = 0;
    for (int i = 0; i < total; ++i)
    {
        const float v = out.getSample (0, i);
        if (! std::isfinite (v)) st.bad = true;
        st.peak = std::max (st.peak, (double) std::abs (v));
        sumSq += (double) v * v;
    }
    st.rms = std::sqrt (sumSq / total);

    WavAudioFormat wav;
    wavFile.getParentDirectory().createDirectory();
    wavFile.deleteFile();
    if (auto stream = wavFile.createOutputStream())
        if (std::unique_ptr<AudioFormatWriter> w { wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0) })
        {
            stream.release();
            w->writeFromAudioSampleBuffer (out, 0, total);
        }
    return st;
}

void report (const String& name, const Stats& st)
{
    std::cout << "STAT|" << name << "|" << Decibels::gainToDecibels (st.peak, -200.0)
              << "|" << Decibels::gainToDecibels (st.rms, -200.0) << "|" << (st.bad ? "BAD" : "ok") << std::endl;
}
} // namespace

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    const File outDir (File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "renders"));
    outDir.createDirectory();

    std::unique_ptr<AudioProcessor> base (createPluginFilter());
    auto& proc = *dynamic_cast<RawTekProcessor*> (base.get());
    proc.setPlayConfigDetails (0, 2, sr, block);
    proc.prepareToPlay (sr, block);

    int failures = 0;
    std::cout << "Programas: " << proc.getNumPrograms() << " (" << RawTekProcessor::getNumFactoryPrograms() << " de fábrica)" << std::endl;
    auto check = [&] (const Stats& st) { if (st.bad || st.peak < 1.0e-3 || st.peak > 2.0) ++failures; };

    for (int prog = 0; prog < proc.getNumPrograms(); ++prog)
    {
        proc.setCurrentProgram (prog);
        const auto name = RawTekProcessor::getProgramShortName (prog);
        const auto cat  = RawTekProcessor::getProgramCategory (prog);
        const bool poly = cat == "Factory" && (name.contains ("Chord") || name.contains ("Stab"));
        auto rcat = poly ? String ("Stab") : cat;
        if (proc.apvts.getRawParameterValue ("aA")->load() > 0.1f) rcat = "Drone";
        const auto st = renderAndWrite (proc, rcat,
                                        outDir.getChildFile ("Programs/" + cat + "/" + name.replaceCharacter (' ', '_') + ".wav"));
        report ("Program " + String (prog) + " " + proc.getProgramName (prog), st);
        check (st);
    }

    if (argc > 2)
    {
        const File presetDir (File::getCurrentWorkingDirectory().getChildFile (argv[2]));
        Array<File> files;
        for (const auto& e : RangedDirectoryIterator (presetDir, true, "*.rtkpreset"))
            files.add (e.getFile());
        files.sort();

        for (const auto& f : files)
        {
            if (! proc.loadPresetFile (f)) { std::cout << "LOADFAIL|" << f.getFullPathName() << std::endl; ++failures; continue; }
            const auto cat = f.getParentDirectory().getFileName();
            const auto pattern = proc.apvts.getRawParameterValue ("aA")->load() > 0.1f ? String ("Drone") : cat; // ataque lento: notas largas
            const auto rel = cat + "/" + f.getFileNameWithoutExtension();
            const auto st = renderAndWrite (proc, pattern, outDir.getChildFile (cat + "/" + f.getFileNameWithoutExtension().replaceCharacter (' ', '_') + ".wav"));
            report (rel, st);
            check (st);
        }
    }

    // cargar un preset con matriz y FX para la captura
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        if (RawTekProcessor::getProgramShortName (i) == "Euphoric Lead") { proc.setCurrentProgram (i); renderAndWrite (proc, "Lead", outDir.getChildFile ("_shot.wav")); }

    // editor: crear, pintar y destruir (pestaña MATRIX y FX)
    for (int tab = 0; tab < 2; ++tab)
    {
        proc.apvts.state.setProperty ("uiTab", tab, nullptr);
        proc.apvts.state.setProperty ("uiScale", 1.0, nullptr);
        std::unique_ptr<AudioProcessorEditor> ed (proc.createEditorIfNeeded());
        std::cout << "Editor: " << (ed ? String (ed->getWidth()) + "x" + String (ed->getHeight()) : "none") << std::endl;
        if (ed)
        {
            Image img (Image::ARGB, ed->getWidth(), ed->getHeight(), true);
            Graphics g (img);
            ed->paintEntireComponent (g, true);
            PNGImageFormat png;
            auto f = outDir.getChildFile (tab == 0 ? "editor.png" : "editor_fx.png");
            f.deleteFile();
            if (auto st = f.createOutputStream()) png.writeImageToStream (img, *st);
        }
        proc.editorBeingDeleted (ed.get());
    }

    std::cout << (failures == 0 ? "OK" : "FAILURES: " + std::to_string (failures)) << std::endl;
    return failures == 0 ? 0 : 1;
}

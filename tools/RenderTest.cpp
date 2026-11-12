// Offline test: renders audio through the processor, checks the numbers,
// and saves pictures of the front and back of the interface.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <juce_dsp/juce_dsp.h>
#include <chrono>

using namespace yoko;
static constexpr double SR = 48000.0;

static void set (YokokokoAudioProcessor& p, const juce::String& id, float realValue)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (realValue));
}

static juce::AudioBuffer<float> run (YokokokoAudioProcessor& p, const juce::AudioBuffer<float>& in)
{
    juce::AudioBuffer<float> out (in);
    juce::MidiBuffer midi;
    for (int pos = 0; pos < out.getNumSamples(); pos += 512)
    {
        const int n = juce::jmin (512, out.getNumSamples() - pos);
        juce::AudioBuffer<float> blk (out.getArrayOfWritePointers(), 2, pos, n);
        p.processBlock (blk, midi);
    }
    return out;
}

static std::unique_ptr<YokokokoAudioProcessor> fresh()
{
    auto p = std::make_unique<YokokokoAudioProcessor>();
    p->setPlayConfigDetails (2, 2, SR, 512);
    p->prepareToPlay (SR, 512);
    return p;
}

// zero-crossing based frequency estimate over a region
static double freqOf (const float* x, int start, int len)
{
    int crossings = 0; int first = -1, last = -1;
    for (int i = start + 1; i < start + len; ++i)
        if (x[i - 1] <= 0 && x[i] > 0) { if (first < 0) first = i; last = i; ++crossings; }
    return crossings > 1 ? (crossings - 1) * SR / (last - first) : 0.0;
}

static double rms (const float* x, int start, int len)
{
    double s = 0; for (int i = start; i < start + len; ++i) s += (double) x[i] * x[i];
    return std::sqrt (s / len);
}

static void writeWav (const juce::AudioBuffer<float>& b, const juce::File& f)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream().release());
    auto w = wav.createWriterFor (os, juce::AudioFormatWriterOptions().withSampleRate (SR).withNumChannels (2).withBitsPerSample (24));
    if (w) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir (argc > 1 ? argv[1] : "/tmp");
    int failures = 0;
    auto check = [&] (bool ok, const juce::String& what) { std::printf ("%s  %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8()); if (! ok) ++failures; };

    // 1. unity delay time: a click comes back 500 ms later -------------------------------
    {
        auto p = fresh();
        set (*p, ID::int1, 0.5f); set (*p, ID::int2, 0.5f); set (*p, ID::regen, 0.0f); set (*p, ID::mix, 1.0f);
        juce::AudioBuffer<float> in (2, (int) SR * 2); in.clear();
        for (int i = 0; i < 48; ++i) in.setSample (0, 4800 + i, 0.8f), in.setSample (1, 4800 + i, 0.8f);
        auto out = run (*p, in);
        int peak = 0; float pv = 0;
        for (int i = 6000; i < out.getNumSamples(); ++i) if (std::abs (out.getSample (0, i)) > pv) { pv = std::abs (out.getSample (0, i)); peak = i; }
        const double ms = (peak - 4800) / SR * 1000.0;
        check (std::abs (ms - 500.0) < 3.0, "echo arrives after " + juce::String (ms, 1) + " ms (expect 500)");
    }

    // 2. octave step: what's in the chips comes out an octave up ------------------------------
    {
        auto p = fresh();
        set (*p, ID::int1, 0.9f); set (*p, ID::int2, 0.5f); set (*p, ID::regen, 0.0f); set (*p, ID::mix, 1.0f); set (*p, ID::lpf, 1.0f);
        juce::AudioBuffer<float> in (2, (int) SR * 2); in.clear();
        for (int i = 0; i < (int) (SR * 0.5); ++i) { const float v = 0.4f * (float) std::sin (2 * kPi * 220.0 * i / SR); in.setSample (0, i, v); in.setSample (1, i, v); }
        auto out = run (*p, in);
        const double f = freqOf (out.getReadPointer (0), (int) (SR * 0.52), (int) (SR * 0.2));
        check (std::abs (f - 440.0) < 6.0, "step 2 plays " + juce::String (f, 1) + " Hz from a 220 Hz input (expect 440)");
    }

    // 3. fifth up -------------------------------------------------------------------------------
    {
        auto p = fresh();
        set (*p, ID::int1, 0.8f); set (*p, ID::regen, 0.0f); set (*p, ID::mix, 1.0f); set (*p, ID::lpf, 1.0f);
        juce::AudioBuffer<float> in (2, (int) SR * 2); in.clear();
        for (int i = 0; i < (int) (SR * 0.5); ++i) { const float v = 0.4f * (float) std::sin (2 * kPi * 220.0 * i / SR); in.setSample (0, i, v); in.setSample (1, i, v); }
        auto out = run (*p, in);
        const double f = freqOf (out.getReadPointer (0), (int) (SR * 0.52), (int) (SR * 0.25));
        check (std::abs (f - 220.0 * 1.4983) < 6.0, "fifth up plays " + juce::String (f, 1) + " Hz (expect 329.6)");
    }

    // 4. runaway feedback stays bounded ----------------------------------------------------------------
    {
        auto p = fresh();
        set (*p, ID::regen, 1.0f); set (*p, ID::mix, 1.0f); set (*p, ID::time, 300.0f);
        juce::AudioBuffer<float> in (2, (int) SR * 20); in.clear();
        for (int i = 0; i < 4800; ++i) { const float v = 0.5f * (float) std::sin (2 * kPi * 330.0 * i / SR); in.setSample (0, i, v); in.setSample (1, i, v); }
        auto out = run (*p, in);
        bool finite = true; float mx = 0;
        for (int i = 0; i < out.getNumSamples(); ++i) { const float v = out.getSample (0, i); finite &= std::isfinite (v); mx = std::max (mx, std::abs (v)); }
        const double late = rms (out.getReadPointer (0), (int) (SR * 18), (int) SR);
        check (finite && mx < 2.0f, "full feedback peaks at " + juce::String (mx, 2) + " with no NaNs");
        check (late > 0.02, "full feedback self-oscillates (rms after 18 s: " + juce::String (late, 3) + ")");
    }

    // 5. slowdown at 2 octaves down: long, grainy, still clean numbers ---------------------------------------
    {
        auto p = fresh();
        set (*p, ID::time, 4000.0f); set (*p, ID::slowdown, 1.0f); set (*p, ID::int1, 0.0f); set (*p, ID::int2, 0.0f);
        set (*p, ID::len1, 2.0f); set (*p, ID::mix, 1.0f); set (*p, ID::regen, 0.0f);
        juce::AudioBuffer<float> in (2, (int) SR * 6); in.clear();
        for (int i = 0; i < (int) SR; ++i) { const float v = 0.3f * (float) std::sin (2 * kPi * 600.0 * i / SR); in.setSample (0, i, v); in.setSample (1, i, v); }
        auto out = run (*p, in);
        bool finite = true; for (int i = 0; i < out.getNumSamples(); ++i) finite &= std::isfinite (out.getSample (0, i));
        const double longest = 8192.0 / p->currentClockHz.load();
        check (finite && longest > 30.0, "slowdown + 2 octaves down reaches " + juce::String (longest, 1) + " s of delay");
    }

    // 6. CPU at the fastest clock (40 ms, 2 octaves up) ---------------------------------------------------------
    {
        auto p = fresh();
        set (*p, ID::time, 40.0f); set (*p, ID::int1, 1.0f); set (*p, ID::int2, 1.0f); set (*p, ID::len1, 2.0f);
        juce::AudioBuffer<float> in (2, (int) SR * 10); in.clear();
        for (int i = 0; i < in.getNumSamples(); ++i) { const float v = 0.3f * (float) std::sin (2 * kPi * 300.0 * i / SR); in.setSample (0, i, v); in.setSample (1, i, v); }
        auto t0 = std::chrono::steady_clock::now();
        auto out = run (*p, in);
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        check (secs < 1.0, "10 s of audio at the fastest clock took " + juce::String (secs, 2) + " s to process");
    }

    // 7. the rest of the switches don't break anything --------------------------------------------------------------
    {
        auto p = fresh();
        juce::AudioBuffer<float> in (2, (int) SR * 4);
        juce::Random r (3);
        for (int i = 0; i < in.getNumSamples(); ++i) { const float v = 0.3f * (r.nextFloat() * 2 - 1); in.setSample (0, i, v); in.setSample (1, i, v); }
        bool finite = true;
        for (auto id : { ID::modulation, ID::rampLpf, ID::rampRegen, ID::rampGlide, ID::step, ID::momentary })
        {
            set (*p, id, 1.0f); set (*p, ID::glide, 0.6f); set (*p, ID::len2, 2.0f);
            auto out = run (*p, in);
            for (int i = 0; i < out.getNumSamples(); ++i) finite &= std::isfinite (out.getSample (0, i)) && std::abs (out.getSample (0, i)) < 3.0f;
        }
        set (*p, ID::on, 0.0f); set (*p, ID::trails, 0.0f);
        in.clear();
        auto out = run (*p, in);
        check (finite, "modulation, ramps, step and glide all run clean");
        check (rms (out.getReadPointer (0), (int) SR, (int) SR * 3) < 1.0e-4, "off without trails is silent");
    }

    // 8. a demo to listen to: plucked notes through the default settings ------------------------------------------------
    {
        auto p = fresh();
        set (*p, ID::time, 500.0f); set (*p, ID::regen, 0.55f); set (*p, ID::mix, 0.5f); set (*p, ID::glide, 0.25f);
        juce::AudioBuffer<float> in (2, (int) (SR * 12)); in.clear();
        const double notes[] = { 196.0, 246.9, 293.7, 392.0, 293.7, 246.9 };
        juce::Random r (7);
        for (int k = 0; k < 6; ++k)
        {
            const int start = (int) (SR * (0.25 + k * 0.5));
            const int period = (int) (SR / notes[k]);
            std::vector<float> ks ((size_t) period);
            for (auto& v : ks) v = r.nextFloat() * 2 - 1;
            for (int i = 0; i < (int) (SR * 0.5); ++i)
            {
                const size_t a = (size_t) (i % period), b = (size_t) ((i + 1) % period);
                ks[a] = 0.497f * (ks[a] + ks[b]);
                const float v = 0.35f * ks[a];
                in.addSample (0, start + i, v); in.addSample (1, start + i, v);
            }
        }
        writeWav (run (*p, in), outDir.getChildFile ("YokokoOnono-demo.wav"));
        std::printf ("wrote demo\n");
    }

    // 9. pictures of the interface ---------------------------------------------------------------------------------------------
    {
        auto p = fresh();
        std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
        auto* e = dynamic_cast<YokokokoAudioProcessorEditor*> (ed.get());
        auto shoot = [&] (const juce::String& name)
        {
            e->refreshLabels();
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
            juce::FileOutputStream fos (outDir.getChildFile (name));
            fos.setPosition (0); fos.truncate();
            juce::PNGImageFormat().writeImageToStream (img, fos);
        };
        shoot ("YokokoOnono-front.png");
        e->showBack (true);
        shoot ("YokokoOnono-back.png");
        set (*p, ID::modulation, 1.0f);
        e->showBack (false);
        {
            juce::AudioBuffer<float> b (2, 512); b.clear(); juce::MidiBuffer m; p->processBlock (b, m);
            e->resized();
        }
        shoot ("YokokoOnono-front-modulation.png");
        std::printf ("wrote pictures\n");
    }

    std::printf ("%d failures\n", failures);
    return failures;
}

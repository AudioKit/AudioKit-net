//
//  EnsembleOscillator.cpp
//  ExtendingAudioKit
//
//  Created by Shane Dunne on 2018-04-02.
//  Copyright © 2018 Shane Dunne & Associates. All rights reserved.
//

#include "EnsembleOscillator.hpp"
#include <math.h>
#include <stdio.h>
#include <random>

namespace AudioKitCore
{

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    void EnsembleOscillator::init(double sampleRate, WaveStack* pStack)
    {
        sampleRateHz = sampleRate;
        pWaveStack = pStack;
        phases = 1;
        freqSpread = 0.0f;
        phaseDeltaMul = 1.0f;
        for (int i=0; i < maxPhases; i++)
        {
            phase[i] = dis(gen);
            octave[i] = 0;
            phaseDelta[i] = 0.0f;
            rightGain[i] = leftGain[i] = 0.5f;
        }
    }

    void EnsembleOscillator::setPhases(int nPhases)
    {
        if (nPhases < 0) nPhases = 0;
        if (nPhases > maxPhases) nPhases = maxPhases;
        phases = nPhases;
    }

    void EnsembleOscillator::setPanSpread(float panSpread)
    {
        if (phases == 0) return;

        if (panSpread < 0.0f) panSpread = 0.0f;
        if (panSpread > 1.0f) panSpread = 1.0f;

        // compute left and right gains (incorporating overall 1/phases scale factor)
        float baseGain = 1.0f / phases;
        if (phases == 1)
        {
            // single-phase case: no panning
            leftGain[0] = rightGain[0] = 0.5f * baseGain;
            return;
        }

        // multi-phase case: with panning
        float pan = -panSpread;  // -1 = full left, 0 = balanced, +1 = full right
        float deltaPan = 2.0f * panSpread / (phases - 1);
        for (int i=0; i < phases; i++)
        {
            float rightFrac = 0.5f * (pan + 1.0f);
            rightGain[i] = baseGain * rightFrac;
            leftGain[i] = baseGain * (1.0f - rightFrac);
            pan += deltaPan;
        }
    }
    
    void EnsembleOscillator::setFrequency(float frequency)
    {
        if (phases == 0) return;

        // First, compute the normalized center frequency (all we need for one phase)
        double normalizedFrequency = double(frequency) / sampleRateHz;
        if (phases == 1)
        {
            // single phase case: just set normalized center frequency
            octave[0] = 0;
            phaseDelta[0] = (float)normalizedFrequency;
            int length = 1 << WaveStack::maxBits;
            while (phaseDelta[0] * length >= 1.0f)
            {
                octave[0]++;
                length >>= 1;
            }
            //printf("%f Hz oct %d\n", frequency, octave[0]);
            return;
        }

        // Multiplier for full step between adjacent phases
        double deltaMultiplier = pow(2.0, (double(freqSpread) / (phases-1)) / 1200.0);
        // And also a multiplier to go in half-steps
        double halfDeltaMultiplier = pow(2.0, (double(freqSpread) / (phases-1)) / 2400.0);

        // multi-phase case: step down to lowest voice (phases-1 half-steps)
        for (int i=0; i < (phases-1); i++)
            normalizedFrequency /= halfDeltaMultiplier;
        // set each phase's normalized frequency, stepping up by full steps
        for (int i=0; i < phases; i++)
        {
            octave[i] = 0;
            phaseDelta[i] = (float)normalizedFrequency;
            normalizedFrequency *= deltaMultiplier;
            int length = 1 << WaveStack::maxBits;
            while (phaseDelta[i] * length >= 1.0f)
            {
                octave[i]++;
                length >>= 1;
            }
        }
        //printf("%f Hz oct %d\n", frequency, octave[0]);
    }

    // Mono output: no panning
    float EnsembleOscillator::getSample()
    {
        if (phases == 0) return 0.0f;

        float gain = 1.0f / phases;
        float sample = 0.0f;

        for (int i=0; i < phases; i++)
        {
            sample += gain * pWaveStack->interp(octave[i], phase[i]);
            phase[i] += phaseDeltaMul * phaseDelta[i];
            if (phase[i] >= 1.0f) phase[i] -= 1.0f;
        }
        return sample;
    }

    // Stereo output: with panning. Outputs are summed into caller-supplied variables.
    void EnsembleOscillator::getSamples(float* pLeft, float* pRight, float gain)
    {
        float leftSample = 0.0f;
        float rightSample = 0.0f;

        for (int i=0; i < phases; i++)
        {
            float sample = pWaveStack->interp(octave[i], phase[i]);
            phase[i] += phaseDeltaMul * phaseDelta[i];
            if (phase[i] >= 1.0f) phase[i] -= 1.0f;

            leftSample += gain * leftGain[i] * sample;
            rightSample += gain * rightGain[i] * sample;
        }
        *pLeft += leftSample;
        *pRight += rightSample;
    }
}

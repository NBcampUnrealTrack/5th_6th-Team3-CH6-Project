#pragma once

#include "CoreMinimal.h"

class USoundWave;

/**
 * Audio silence trimming utility (front/back)
 */
class FAudioTrimUtils
{
public:
    /**
     * Create a new trimmed USoundWave by removing only silence at the ends.
     * - Middle silence is preserved.
     * - Processed based on 16-bit PCM (Interleaved) input.
     *
     * @param Source                   Original sound wave (requires RawPCMData)
     * @param vadThreshold            RMS threshold (default: 0.001f)
     * @param preRollSeconds           Start margin seconds (default: 0.1f)
     * @param postRollSeconds          End margin seconds (default: 0.1f)
     * @param frameDurationSeconds     Frame length seconds (default: 0.02f)
     * @return Trimmed new USoundWave (nullptr on failure, or original if no trimming occurred)
     */
    static USoundWave* TrimSilenceFromEnds(const USoundWave* Source,
                                           float vadThreshold = 0.05f,
                                           float preRollSeconds = 0.1f,
                                           float postRollSeconds = 0.1f,
                                           float frameDurationSeconds = 0.02f);
}; 
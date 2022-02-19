#ifndef CONSTANTS_H
#define CONSTANTS_H

struct Constants
{
    struct Settings
    {
        static inline const char* saveHashParametersOnQuit = "SaveParams";
        static inline const char* saveWindowSizeOnQuit = "SaveWindowSize";
        static inline const char* selectTextOnFocus = "SelectTextOnFocus";
        static inline const char* windowWidth = "WindowWidth";
        static inline const char* windowHeight = "WindowHeight";
        static inline const char* hashAlgo = "HashAlgoId";
        static inline const char* timeCost = "TimeCost";
        static inline const char* memoryCost = "MemoryCostMiB";
        static inline const char* parallelism = "Parallelism";
        static inline const char* hashLength = "HashLength";

        struct DefaultValues
        {
            static constexpr bool saveHashParametersOnQuit = true;
            static constexpr bool saveWindowSizeOnQuit = true;
            static constexpr bool selectTextOnFocus = true;
            static constexpr int hashAlgo = 0;
            static constexpr int timeCost = 16;
            static constexpr int memoryCostMiB = 32;
            static constexpr int parallelism = 2;
            static constexpr int hashLength = 64;
        };
    };

    // Tiny lookup table for english plural suffix (accessible via a boolean check as an index).
    static inline const char* plural[] = { "", "s" };

    static inline const char* appName = "Argon2GUI";
    static inline const char* appVersion = "1.0.1";
    static inline const char* orgName = "Glitched Polygons";
    static inline const char* orgDomain = "glitchedpolygons.com";
};

#endif // CONSTANTS_H

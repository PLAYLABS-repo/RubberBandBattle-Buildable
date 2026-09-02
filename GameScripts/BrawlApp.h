#pragma once

namespace RubberBandBattle
{
    enum BrawlMode
    {
        BRAWL_MODE_GAME,
        BRAWL_MODE_DEBUG
    };

    void RunBrawlApp(BrawlMode mode);
}

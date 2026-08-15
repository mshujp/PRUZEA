#pragma once

#include "SystemUI.h"

namespace PRUZEA
{

class SystemUI320x240 : public SystemUI
{
private:
    static constexpr uint16_t ITEMS_PER_PAGE = 4;

protected:
    uint16_t getItemsPerPage() const override { return ITEMS_PER_PAGE; }

    void drawSplash(Graphics& graphics) override;
    void drawSelect(Graphics& graphics) override;
    void drawInfo(Graphics& graphics) override;
    void drawShutdownConfirm(Graphics& graphics) override;
    bool supportsGameDataDelete() const override { return true; }
    void drawDeleteGameDataConfirm(Graphics& graphics) override;

public:
    static constexpr uint16_t SCREEEN_WIDTH = 320;
    static constexpr uint16_t SCREEEN_HIGHT = 240;

    SystemUI320x240();

    uint16_t getLogicalScreenWidth() const override { return SystemUI320x240::SCREEEN_WIDTH; }
    uint16_t getLogicalScreenHeight() const override { return SystemUI320x240::SCREEEN_HIGHT; }
    uint16_t getTargetScreenWidth() const override { return getLogicalScreenWidth(); }
    uint16_t getTargetScreenHeight() const override { return getLogicalScreenHeight(); }

    void drawPowerOffReady(Graphics& graphics) override;
    void drawLowBttery(Graphics& graphics) override;
};

} // namespace PRUZEA

#pragma once

#include "global.h"
#include "pci.h"
#include "pmu.h"
#include "utils.h"

namespace pcm
{

class CHA{
  public:
    int eventCount;
    
    CHA();
    virtual ~CHA();
    bool program(std::string configStr);
    void run();
    void getCounter(std::vector<std::vector<uint64>>& M, int counterId);
    void print();
    void initFreeze();
  private:
    uint32 getMaxNumOfCBoxes() const;
    //void initFreeze();

    std::vector<std::vector<UncorePMU>> cboPMUs;
    std::vector<std::string> names;

};

inline auto CX_MSR_PMON_BOX_FILTER(uint32 Cbo)
    { return SPR_CHA0_MSR_PMON_BOX_FILTER + SPR_CHA_MSR_STEP * Cbo; }
inline auto CX_MSR_PMON_BOX_CTL(uint32 Cbo)
    { return SPR_CHA0_MSR_PMON_BOX_CTRL + SPR_CHA_MSR_STEP * Cbo; }
inline auto CX_MSR_PMON_CTLY(uint32 Cbo, uint32 Ctl)
    { return SPR_CHA0_MSR_PMON_CTL0 + SPR_CHA_MSR_STEP * Cbo + Ctl; }
inline auto CX_MSR_PMON_CTRY(uint32 Cbo, uint32 Ctr)
    { return SPR_CHA0_MSR_PMON_CTR0 + SPR_CHA_MSR_STEP * Cbo + Ctr; }

inline UncorePMU makeCHAPMU(std::shared_ptr<SafeMsrHandle>& handle, uint32 cbo);

}   // namespace pcm

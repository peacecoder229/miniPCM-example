#pragma once

#include "global.h"
#include "pci.h"
#include "pmu.h"
#include "utils.h"

namespace pcm
{

class IIO{
  public:
    IIO();
    virtual ~IIO(){};
    bool program(std::string configStr);
    void run();
    void getCounter(std::vector<std::vector<uint64>>& M, int counterId);
    void print();
    void printFR();

  private:
    void initFreeze();

    int eventCount;
    std::vector<std::vector<UncorePMU>> iioPMUs;
    std::vector<std::vector<FreeRunBWCounters>> iioFRCtrs;
    std::vector<std::string> names;
};

inline UncorePMU makeIIOPMU(std::shared_ptr<SafeMsrHandle>& handle, int unit);

}   // namespace pcm
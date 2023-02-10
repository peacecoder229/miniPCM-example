#include "iio.h"

namespace pcm
{

inline UncorePMU makeIIOPMU(std::shared_ptr<SafeMsrHandle>& handle, int unit)
{
    return UncorePMU(
                    std::make_shared<MSRRegister>(handle, SPR_M2IOSF_IIO_UNIT_CTL + SPR_M2IOSF_REG_STEP * unit),
                    std::make_shared<MSRRegister>(handle, SPR_M2IOSF_IIO_CTL0 + SPR_M2IOSF_REG_STEP * unit + 0),
                    std::make_shared<MSRRegister>(handle, SPR_M2IOSF_IIO_CTL0 + SPR_M2IOSF_REG_STEP * unit + 1),
                    std::make_shared<MSRRegister>(handle, SPR_M2IOSF_IIO_CTL0 + SPR_M2IOSF_REG_STEP * unit + 2),
                    std::make_shared<MSRRegister>(handle, SPR_M2IOSF_IIO_CTL0 + SPR_M2IOSF_REG_STEP * unit + 3),
                    std::make_shared<MSRRegister>(handle, SPR_M2IOSF_IIO_CTR0 + SPR_M2IOSF_REG_STEP * unit + 0),
                    std::make_shared<MSRRegister>(handle, SPR_M2IOSF_IIO_CTR0 + SPR_M2IOSF_REG_STEP * unit + 1),
                    std::make_shared<MSRRegister>(handle, SPR_M2IOSF_IIO_CTR0 + SPR_M2IOSF_REG_STEP * unit + 2),
                    std::make_shared<MSRRegister>(handle, SPR_M2IOSF_IIO_CTR0 + SPR_M2IOSF_REG_STEP * unit + 3));
}

IIO::IIO()
{
    eventCount = 0;
    iioPMUs.resize(2);

    for (uint32 s = 0; s < iioPMUs.size(); ++s)
    {
        auto handle = (s == 0) ? std::make_shared<SafeMsrHandle>(0) 
                               : std::make_shared<SafeMsrHandle>(getNumCores()-1);
        for (uint32 unit = 0; unit < SPR_M2IOSF_NUM; ++unit)
        {
            iioPMUs[s].push_back(makeIIOPMU(handle, unit));
        }
    }

    initFreeze();
}

bool IIO::program(std::string configStr)
{
    if (eventCount == SERVER_UNCORE_COUNTER_MAX_CONTROLLERS)
    {
        std::cerr << "too many events for CHA unit" << std::endl;
        return false;
    }

    const auto configArray = split(configStr, ',');
    bool fixed = false; // TODO: find out where fixed is used and implement support
    uint64 event;
    uint32 filter0 = -1;
    for (auto item : configArray)
    {
        std::string f0, f1;
        if (match("config=(0[xX][0-9a-fA-F]+)", item, f0)) {
            event = strtoll(f0.c_str(), NULL, 16);
            std::cout << "Config read " << std::hex << event << "\n";
        }
        else if (match("name=(.+)", item, f1)) {
            names.push_back(f1);
            std::cout << "Name read " << f1 << "\n";
        }
    }

    for(auto& iioPMUsPerSocket : iioPMUs){
        for(auto& iioPMU : iioPMUsPerSocket){
            auto ctrl = iioPMU.counterControl[eventCount];
            if(ctrl.get())
                *ctrl = event;
        }
    }

    eventCount++;
    return true;
}

void IIO::initFreeze()
{
    for (auto& iioPMUsPerSocket : iioPMUs)
        for (auto& iioPMU : iioPMUsPerSocket)
            iioPMU.initFreeze();
}

void IIO::run()
{
    for (auto& iioPMUsPerSocket : iioPMUs)
        for (auto& iioPMU : iioPMUsPerSocket)
            iioPMU.resetUnfreeze();
}

void IIO::getCounter(std::vector<std::vector<uint64>>& M, int counterId)
{
    if (counterId >= eventCount){
        std::cerr << "Trying to read unused counter " << counterId;
        return;
    }

    if (M.size() != iioPMUs.size()) {
        M.resize(iioPMUs.size());
        for(int i = 0; i < iioPMUs.size(); ++i){
            M[i].resize(iioPMUs[i].size());
        }
    }

    for(int i = 0; i < iioPMUs.size(); ++i){
        for(int j = 0; j < iioPMUs[i].size(); ++j){
            iioPMUs[i][j].freeze();
            M[i][j] = *(iioPMUs[i][j].counterValue[counterId]);
            // printf("iioPMU[%d][%d] pmu.counterValue[%d] = %x value = %d\n", i, j, counterId, imcPMUs[i][j].counterValue[counterId], M[i][j]);
            iioPMUs[i][j].unfreeze();
        }
    }
}

void IIO::print()
{
    static std::vector<std::vector<std::vector<uint64>>> M, M_prev;
    if (M.empty()){
        M.resize(eventCount);
        for(int i = 0; i < eventCount; ++i){
            M[i].resize(iioPMUs.size());
                for(int j = 0; j < iioPMUs.size(); ++j){
                    M[i][j].resize(iioPMUs[i].size());
                }
        }

        for(int i = 0; i < iioPMUs.size(); ++i){
            for(int j = 0; j < iioPMUs[i].size(); ++j){
                iioPMUs[i][j].freeze();
                for(int k = 0; k < eventCount; ++k){
                    M[k][i][j] = *(iioPMUs[i][j].counterValue[k]);
                    // printf("iioPMU[%d][%d] pmu.counterValue[%d] = %x value = %d\n", i, j, counterId, imcPMUs[i][j].counterValue[counterId], M[i][j]);
                }
                iioPMUs[i][j].unfreeze();
            }
        }

        M_prev = M;
    }
    
    for(int i = 0; i < iioPMUs.size(); ++i){
        for(int j = 0; j < iioPMUs[i].size(); ++j){
            iioPMUs[i][j].freeze();
            for(int k = 0; k < eventCount; ++k){
                M[k][i][j] = *(iioPMUs[i][j].counterValue[k]);
                // printf("iioPMU[%d][%d] pmu.counterValue[%d] = %x value = %d\n", i, j, counterId, imcPMUs[i][j].counterValue[counterId], M[i][j]);
            }
            iioPMUs[i][j].unfreeze();
        }
    }

    printf("iio:\n");
    for(int soc = 0; soc < 2; soc++){
        printf("  socket %d\n", soc);
        for(int stack = 0; stack < SPR_M2IOSF_NUM; stack++){
            printf("    stack %d\n", stack);
            for(int e = 0; e < eventCount; e++){
                printf("     %d)", e+1);
                std::cout << names[e] << " = " 
                          << M[e][soc][stack] - M_prev[e][soc][stack]
                          << std::endl;
            }
        }
    }

    M_prev = M;
}

} // namespace pcm
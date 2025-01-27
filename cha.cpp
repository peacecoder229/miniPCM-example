#include "cha.h"

namespace pcm
{

inline UncorePMU makeCHAPMU(std::shared_ptr<SafeMsrHandle>& handle, uint32 cbo)
{
    return UncorePMU(std::make_shared<MSRRegister>(handle, CX_MSR_PMON_BOX_CTL(cbo)),
                     std::make_shared<MSRRegister>(handle, CX_MSR_PMON_CTLY(cbo, 0)),
                     std::make_shared<MSRRegister>(handle, CX_MSR_PMON_CTLY(cbo, 1)),
                     std::make_shared<MSRRegister>(handle, CX_MSR_PMON_CTLY(cbo, 2)),
                     std::make_shared<MSRRegister>(handle, CX_MSR_PMON_CTLY(cbo, 3)),
                     std::make_shared<MSRRegister48>(handle, CX_MSR_PMON_CTRY(cbo, 0)),
                     std::make_shared<MSRRegister48>(handle, CX_MSR_PMON_CTRY(cbo, 1)),
                     std::make_shared<MSRRegister48>(handle, CX_MSR_PMON_CTRY(cbo, 2)),
                     std::make_shared<MSRRegister48>(handle, CX_MSR_PMON_CTRY(cbo, 3)),
                     std::shared_ptr<MSRRegister>(),
                     std::shared_ptr<MSRRegister>(),
                     std::make_shared<MSRRegister>(handle, CX_MSR_PMON_BOX_FILTER(cbo)),
                     std::shared_ptr<HWRegister>());
}

CHA::CHA()
{
    eventCount = 0;
    cboPMUs.resize(sockets);

    for (uint32 s = 0; s < cboPMUs.size(); ++s)
    {
        auto handle = (s == 0) ? std::make_shared<SafeMsrHandle>(0) 
                               : std::make_shared<SafeMsrHandle>(getNumCores()-1);
        for (uint32 cbo = 0; cbo < getMaxNumOfCBoxes(); ++cbo)
        {
            cboPMUs[s].push_back(makeCHAPMU(handle, cbo));
        }
    }

    initFreeze();
}

CHA::~CHA(){}

PciHandleType * getDeviceHandle(uint32 vendorId, uint32 deviceId)
{
    const std::vector<MCFGRecord> & mcfg = PciHandle::getMCFGRecords();

    for(uint32 s = 0; s < (uint32)mcfg.size(); ++s)
    {
        for (uint32 bus = (uint32)mcfg[s].startBusNumber; bus <= (uint32)mcfg[s].endBusNumber; ++bus)
        {
            for (uint32 device = 0; device < 0x20; ++device)
            {
                for (uint32 function = 0; function < 0x8; ++function)
                {
                    if (PciHandleType::exists(mcfg[s].PCISegmentGroupNumber, bus, device, function))
                    {
                        PciHandleType * h = new PciHandleType(mcfg[s].PCISegmentGroupNumber, bus, device, function);
                        uint32 value;
                        h->read32(0, &value);
                        const uint32 vid = value & 0xffff;
                        const uint32 did = (value >> 16) & 0xffff;
                        if (vid == vendorId && did == deviceId)
                            return h;
                        delete h;
                    }
                }
            }
        }
    }
    return NULL;
}

inline uint32 weight32(uint32 n)
{
    uint32 count = 0;
    while (n)
    {
        n &= (n - 1);
        count++;
    }

    return count;
}

uint32 CHA::getMaxNumOfCBoxes() const
{
    static int num = -1;
    if(num >= 0) return (uint32)num;

    PciHandleType * h = getDeviceHandle(PCM_INTEL_PCI_VENDOR_ID, 0x325b);
    if(h)
    {
        uint32 value;
        h->read32(0x9c, &value);
        num = weight32(value);
        h->read32(0xa0, &value);
        num += weight32(value);
        delete h;
    }

    return num;
}

bool CHA::program(std::string configStr)
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
        std::string f0, f1, f2;
        if (match("config=(0[xX][0-9a-fA-F]+)", item, f0)) {
            event = strtoll(f0.c_str(), NULL, 16);
            std::cout << "Config read " << std::hex << event << "\n";
        }
        else if (match("config1=(0[xX][0-9a-fA-F]+)", item, f1)) {
            filter0 = strtol(f1.c_str(), NULL, 16);
            std::cout  << "Config1 read " << std::hex << filter0 << "\n";
        }
        else if (match("name=(.+)", item, f2)) {
            names.push_back(f2);
            std::cout << "Name read " << f2 << "\n";
        }
    }

    if(filter0 != -1){
        for(auto& cboPMUsPerSocket : cboPMUs){
            for(auto& cboPMU : cboPMUsPerSocket){
                if(cboPMU.filter[0].get())
                    *cboPMU.filter[0] = filter0;
            }
        }
    }

    for(auto& cboPMUsPerSocket : cboPMUs){
        for(auto& cboPMU : cboPMUsPerSocket){
            auto ctrl = cboPMU.counterControl[eventCount];
            if(ctrl.get())
                *ctrl = event;
        }
    }

    eventCount++;
    return true;
}

void CHA::initFreeze()
{
    for (auto& cboPMUsPerSocket : cboPMUs)
        for (auto& cboPMU : cboPMUsPerSocket)
            cboPMU.initFreeze();
}

void CHA::run()
{
    for (auto& cboPMUsPerSocket : cboPMUs)
        for (auto& cboPMU : cboPMUsPerSocket)
            cboPMU.resetUnfreeze();
}

void CHA::getCounter(std::vector<std::vector<uint64>>& M, int counterId)
{
    if (counterId >= eventCount){
        //std::cerr << "Trying to read unused counter " << counterId << std::endl;
        return;
    }

    if (M.size() != cboPMUs.size()) {
        M.resize(cboPMUs.size());
        for(int i = 0; i < cboPMUs.size(); ++i){
            M[i].resize(cboPMUs[i].size());
        }
    }

    for(int i = 0; i < cboPMUs.size(); ++i){
        for(int j = 0; j < cboPMUs[i].size(); ++j){
            cboPMUs[i][j].freeze();
            M[i][j] = *(cboPMUs[i][j].counterValue[counterId]);
            // printf("cboPMU[%d][%d] pmu.counterValue[%d] = %x value = %d\n", i, j, counterId, imcPMUs[i][j].counterValue[counterId], M[i][j]);
            cboPMUs[i][j].unfreeze();
        }
    }
}

void CHA::print()
{
    static std::vector<std::vector<std::vector<uint64>>> M, M_prev;
    uint64 result, prev;
    // double ddrcyclecount = 1e9 *60 / (1/2.4);

    if(eventCount == 0) return;
    if (M.empty()){
        M.resize(eventCount);
        for(int i = 0; i < eventCount; ++i){
            M[i].resize(cboPMUs.size());
                for(int j = 0; j < cboPMUs.size(); ++j){
                    M[i][j].resize(cboPMUs[j].size());
                }
        }

        for(int i = 0; i < cboPMUs.size(); ++i){
            for(int j = 0; j < cboPMUs[i].size(); ++j){
                cboPMUs[i][j].freeze();
                for(int k = 0; k < eventCount; ++k){
                    M[k][i][j] = *(cboPMUs[i][j].counterValue[k]);
                    // printf("imcPMU[%d][%d] pmu.counterValue[%d] = %x value = %d\n", i, j, counterId, cboPMUs[i][j].counterValue[counterId], M[i][j]);
                }
                cboPMUs[i][j].unfreeze();
            }
        }

        M_prev = M;
    }
    
    for(int i = 0; i < cboPMUs.size(); ++i){
        for(int j = 0; j < cboPMUs[i].size(); ++j){
            cboPMUs[i][j].freeze();
            for(int k = 0; k < eventCount; ++k){
                M[k][i][j] = *(cboPMUs[i][j].counterValue[k]);
                // printf("imcPMU[%d][%d] pmu.counterValue[%d] = %x value = %d\n", i, j, counterId, cboPMUs[i][j].counterValue[counterId], M[i][j]);
            }
            cboPMUs[i][j].unfreeze();
        }
    }

    printf("cha:\n");
    for(int soc = 0; soc < 2; soc++){
        printf("  socket %d\n", soc);
            for(int e = 0; e < eventCount; e++){
                printf("   %d)", e+1);
                result = 0;
                prev = 0;
                for(int i = 0; i < M[e][soc].size(); i++){
                    result += M[e][soc][i];
                    prev += M_prev[e][soc][i];
                }
                std::cout << names[e] << " = " 
                          << std::dec << (result - prev)
                          << std::endl;
            }
    }

    M_prev = M;
}

}   // namespace pcm

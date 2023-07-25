#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <regex>
#include <iostream>
#include <unistd.h>

#include "types.h"

namespace pcm
{

enum SupportedCPUModels
    {
        ICX = 106,
        SPR = 143,
        EMR = 207,
        GNR = 173,
        GNR_D = 174,
        GRR = 182,
        SRF = 175,
        UNSUPPORTED = 5555
    };
std::vector<std::string> split(const std::string & str, const char delim);
std::tuple<double, double, double> getMultipliersForModel(uint32 cpu_model);
double calculate_metric(const std::string& formula, std::map<std::string, double>& values, double n_sample_in_sec, bool multiplyFlag);
bool match(const std::string& subtoken, const std::string& sname, std::string& result);
class s_expect : public std::string
{
public:
    explicit s_expect(const char * s) : std::string(s) {}
    explicit s_expect(const std::string & s) : std::string(s) {}
    friend std::istream & operator >> (std::istream & istr, s_expect && s);
    friend std::istream & operator >> (std::istream && istr, s_expect && s);
private:

    void match(std::istream & istr) const;

};

FILE * tryOpen(const char * path, const char * mode);

std::string readSysFS(const char * path, bool silent);

bool writeSysFS(const char * path, const std::string & value, bool silent);

int32 getNumCores();

//extern uint32 cpu_model;
uint32 getCPUModel();



union PCM_CPUID_INFO
{
    int array[4];
    struct { unsigned int eax, ebx, ecx, edx; } reg;
};

inline void pcm_cpuid(int leaf, PCM_CPUID_INFO& info)
{
    __asm__ __volatile__("cpuid" : \
        "=a" (info.reg.eax), "=b" (info.reg.ebx), "=c" (info.reg.ecx), "=d" (info.reg.edx) : "a" (leaf));
}

}   // namespace pcm

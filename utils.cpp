#include "utils.h"

namespace pcm
{

std::vector<std::string> split(const std::string & str, const char delim)
{
    std::string token;
    std::vector<std::string> result;
    std::istringstream strstr(str);
    while (std::getline(strstr, token, delim))
    {
        result.push_back(token);
    }
    return result;
}

std::tuple<double, double, double> getMultipliersForModel(uint32 cpu_model) {
        switch (cpu_model) {
            case GNR:
            case SRF:
                return std::make_tuple(2.0, 4.0, 4.0);
            default:
                return std::make_tuple(1.0, 1.0, 1.0);
        }
}



std::string cpuModelToString(SupportedCPUModels model) 
{
    std::map<SupportedCPUModels, std::string> cpuModelToString;
    cpuModelToString[ICX] = "ICX";
    cpuModelToString[SPR] = "SPR";
    cpuModelToString[EMR] = "EMR";
    cpuModelToString[GNR] = "GNR";
    cpuModelToString[GNR_D] = "GNR_D";
    cpuModelToString[GRR] = "GRR";
    cpuModelToString[SRF] = "SRF";
    cpuModelToString[UNSUPPORTED] = "UNSUPPORTED";

    if (cpuModelToString.find(model) != cpuModelToString.end())
    {
        return cpuModelToString[model];
    }
    else
    {
        return "UNKNOWN";
    }
}


bool match(const std::string& subtoken, const std::string& sname, std::string& result)
{

	std::regex rgx(subtoken);
	std::smatch matched;
	if (std::regex_search(sname.begin(), sname.end(), matched, rgx)){
		std::cout << "match: " << matched[1] << '\n';
		result= matched[1];
		return true;
	}
	return false;

}

void s_expect::match(std::istream & istr) const
{
    istr >> std::noskipws;
    const auto len = length();
    char * buffer = new char[len + 2];
    buffer[0] = 0;
    istr.get(buffer, len+1);
    if (*this != std::string(buffer))
    {
        istr.setstate(std::ios_base::failbit);
    }
    delete [] buffer;
}

inline std::istream & operator >> (std::istream & istr, s_expect && s)
{
    s.match(istr);
    return istr;
}

inline std::istream & operator >> (std::istream && istr, s_expect && s)
{
    s.match(istr);
    return istr;
}

FILE * tryOpen(const char * path, const char * mode)
{
    FILE * f = fopen(path, mode);
    if (!f)
    {
        f = fopen((std::string("/pcm") + path).c_str(), mode);
    }
    return f;
}

std::string readSysFS(const char * path, bool silent = false)
{
    FILE * f = tryOpen(path, "r");
    if (!f)
    {
        if (silent == false) std::cerr << "ERROR: Can not open " << path << " file.\n";
        return std::string();
    }
    char buffer[1024];
    if(NULL == fgets(buffer, 1024, f))
    {
        if (silent == false) std::cerr << "ERROR: Can not read from " << path << ".\n";
        fclose(f);
        return std::string();
    }
    fclose(f);
    return std::string(buffer);
}

bool writeSysFS(const char * path, const std::string & value, bool silent = false)
{
    FILE * f = tryOpen(path, "w");
    if (!f)
    {
        if (silent == false) std::cerr << "ERROR: Can not open " << path << " file.\n";
        return false;
    }
    if (fputs(value.c_str(), f) < 0)
    {
        if (silent == false) std::cerr << "ERROR: Can not write to " << path << ".\n";
        fclose(f);
        return false;
    }
    fclose(f);
    return true;
}

uint32 getCPUModel() {
    PCM_CPUID_INFO cpuinfo; // You'll need to define cpuinfo

    pcm_cpuid(1, cpuinfo);
    uint32 cpu_family = (((cpuinfo.array[0]) >> 8) & 0xf) | ((cpuinfo.array[0] & 0xf00000) >> 16);
    uint32 model = (((cpuinfo.array[0]) & 0xf0) >> 4) | ((cpuinfo.array[0] & 0xf0000) >> 12);
    uint32 cpu_stepping = cpuinfo.array[0] & 0x0f;

    if (cpu_family != 6)
    {
        std::cerr << "CPU Family is not supported by PCM" << " CPU Family: " << cpu_family << "\n";
        return UNSUPPORTED;  // Or another value to indicate error
    }

    return model;
}



int32 getNumCores()
{
    static int num_cores = -1;
    if(num_cores >= 0) return num_cores;

    // Only works for linux systems
    num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    
    return num_cores;
}

}   // namespace pcm

#include <trafficinfo.h>
#include <format>

TrafficInfo::FormatString TrafficInfo::FormatSpeed(uint32_t bytes, const FormatString& fmtStr, const SpeedUnits & uints)
{
  // https://unicode-table.com/en/2192/
  auto iter = uints.cbegin();
  uint64_t size = 1;
  // 2^11 >> 10 = 2, 2^10 >> 10 = 1, 2^10 | 2^11-1 >> 10 = 1
  for (auto const kb = (bytes >> 10); size <= kb; ++iter) { size <<= 10; }

  return std::vformat(fmtStr, std::make_wformat_args(
    static_cast<float>(bytes) / size, *iter
  ));
  // m_SysInfo[upload ? 0 : 1] = std::vformat(m_StrFmt[upload ? 0 : 1], std::make_format_args(static_cast<float>(bytes) / size, *u));
}

